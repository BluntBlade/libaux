#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "str/string.h"

typedef unsigned char char_t;

typedef struct NSTR {
    uint32_t is_ref:1;          // 片段标志位：0 表示字符串，1 表示片段引用。
    uint32_t need_free:1;       // 是否释放内存：0 表示不需要，1 表示需要。
    uint32_t encoding:8;        // 编码方案代号，最多支持 255 种编码。
    uint32_t unused:22;         // 未使用位域，保持边界对齐。

    uint32_t chars;             // 编码后的字符个数。
    uint32_t bytes;             // 占用内存字节数。

    union {
        uint32_t refs;          // 片段引用计数，生成字符串时置 1 ，表示对自身的引用。减到 0 时释放内存。
        uint32_t offset;        // 片段在源字符串内存区的起始位置（相对于起点的字节数）。
    };

    union {
        char_t        buf[4];   // 单字节字符数据内存区，包含结尾的 NUL 字符。
        struct NSTR * src;      // 指向被引用的字符串，其 buf 与 offset 相加得到片段所在内存起点位置。
    };
} nstr_t;

// ---- ASCII related functions ---- //

static uint32_t measure_ascii(void * pos)
{
    return 1;
} // measure_ascii

static void * locate_ascii(void * begin, void * end, uint32_t index, uint32_t * chars)
{
    uint32_t bytes = end - begin;
    if (index >= bytes) {
        *chars = 0;
        return end;
    } // if
    *chars = index;
    return begin + index;
} // locate_ascii

static uint32_t count_ascii(void * begin, void * end)
{
    return end - begin;
} // count_ascii

static bool verify_ascii(void * begin, void * end)
{
    void * pos = memchr(begin, 0, end - begin);
    return (pos == NULL);
} // verify_ascii

// ---- UTF-8 related functions ---- //

// UTF-8 encodes code points in one to four bytes, depending on the value of the code point. In the following table, the characters u to z are replaced by the bits of the code point, from the positions U+uvwxyz:
// Code point ↔  UTF-8 conversion 
// First Code Point    Last Code Point    Byte 1      Byte 2      Byte 3      Byte 4
// U+0000              U+007F             0yyyzzzz
// U+0080              U+07FF             110xxxyy    10yyzzzz
// U+0800              U+FFFF             1110wwww    10xxxxyy    10yyzzzz
// U+010000            U+10FFFF           11110uvv    10vvwwww    10xxxxyy    10yyzzzz

static uint32_t measure_utf8(void * pos)
{
    char_t ch = ((char_t *)pos)[0];
    if ((ch & 0x80) == 0) return 1;
    if (((ch <<= 1) & 0x80) == 0) return 0;
    if (((ch <<= 1) & 0x80) == 0) return 2;
    if (((ch <<= 1) & 0x80) == 0) return 3;
    if (((ch <<= 1) & 0x80) == 0) return 4;
    return 0;
} // measure_utf8

static void * locate_utf8(void * begin, void * end, uint32_t index, uint32_t * chars)
{
    uint32_t cnt = 0;
    void * pos = begin;
    while (cnt < index && pos < end) {
        bytes = measure_utf8(pos);
        if (bytes == 0) break;
        cnt += 1;
        pos += bytes;
    } // while
    *chars = cnt;
    return pos;
} // locate_utf8

static uint32_t count_utf8(void * begin, void * end)
{
    uint32_t cnt = 0;
    locate_utf8(begin, end, 0, &cnt);
    return cnt;
} // count_utf8

static bool verify_utf8(void * begin, void * end)
{
    uint32_t bytes = 0;
    void * pos = begin;
    while (pos < end) {
        bytes = measure_utf8(pos);
        if (bytes == 0) return false;
        pos += bytes;
    } // while
    return true;
} // verify_utf8

typedef uint32_t (*measure_t)(void * pos);
static measure_t measure[NSTR_ENCODING_COUNT] = {
    &measure_ascii,
    &measure_utf8,
};

typedef void * (*locate_t)(void * begin, void * end, uint32_t index, uint32_t * chars);
static locate_t locate[NSTR_ENCODING_COUNT] = {
    &locate_ascii,
    &locate_utf8,
};

typedef uint32_t (*count_t)(void * begin, void * end);
static count_t count[NSTR_ENCODING_COUNT] = {
    &count_ascii,
    &count_utf8,
};

typedef bool (*verify_t)(void * begin, void * end);
static verify_t verify[NSTR_ENCODING_COUNT] = {
    &verify_ascii,
    &verify_utf8,
};

static nstr_t blank_strings[STR_ENCODING_COUNT] = {
    {.encoding = STR_ASCII},
    {.encoding = STR_UTF8},
};

inline static void * real_buffer(nstr_p s)
{
    return (s->is_ref) ? s->src->buf + s->offset : s->buf;
} // real_buffer

inline static nstr_p real_string(nstr_p s)
{
    return (s->is_ref) ? s->src : s;
} // real_string

// 需要分配内存字节数
size_t nstr_object_size(uint32_t bytes)
{
    return sizeof(nstr_t) + bytes;
} // nstr_object_size

inline static void init_string(nstr_p s, bool need_free, uint32_t bytes, uint32_t chars, uint32_t encoding)
{
    s->refs = 1;  // 引用自身。
    s->bytes = bytes;
    s->chars = chars;
    s->encoding = encoding;
    s->need_free = need_free ? 1 : 0;
    s->is_ref = 0;
} // init_string

// 从原生字符串生成新字符串
nstr_p nstr_new(void * src, uint32_t bytes, str_encoding_t encoding)
{
    nstr_p new = NULL;

    if (bytes == 0) return nstr_blank(encoding);
    if ((new = calloc(1, nstr_object_size(bytes)))) {
        memcpy(new->buf, src, bytes);
        init_string(new, STR_NEED_FREE, bytes, count[encoding](src, src + bytes), encoding);
    } // if
    return new;
} // nstr_new

// 从源字符串（或片段引用）生成新字符串
nstr_p nstr_clone(nstr_p s)
{
    return nstr_new(real_buffer(s), s->bytes, s->encoding);
} // nstr_clone

// 删除字符串
void nstr_delete(nstr_p * ps)
{
    nstr_p s = *ps;

    if (! s) return;
    if (s->is_ref) {
        s = s->src;
        free(*ps); // 释放片段引用
    } // if

    if (s == blank_strings[s->encoding]) return; // 空字符串不需要释放。

    s->refs -= 1;
    if (s->refs == 0 && s->need_free == 1) {
        free(s); // 释放字符串
    } // if

    *ps = NULL; // 防止野指针
} // nstr_delete

// 删除切分后的字符串数组
void nstr_delete_strings(nstr_p * as, int n)
{
    while (--n >= 0) nstr_delete(as[n]);
    free(as);
} // nstr_delete_strings

// 返回编码方案代号
uint32_t nstr_encoding(nstr_p s)
{
    return s->encoding;
} // nstr_encoding

// 返回字节数，不包含最后的 NUL 字符
uint32_t nstr_bytes(nstr_p s)
{
    return s->bytes;
} // nstr_bytes

// 返回字符数，不包含最后的 NUL 字符
uint32_t nstr_chars(nstr_p s)
{
    return s->chars;
} // nstr_chars

// 返回原生字符串指针（片段引用情况下会生成一个新字符串）
void * nstr_to_cstr(nstr_p * ps)
{
    nstr_p n = NULL;
    nstr_p s = *ps;

    if (s->is_ref) {
        n = nstr_new(s->src->buf + s->offset, s->bytes, s->encoding);
        if (! n) return NULL;
        *ps = n;
    } // if
    return (*ps)->buf;
} // nstr_to_cstr

// 返回片段引用数（返回 0 表示这是一个片段引用）
uint32_t nstr_refs(nstr_p s)
{
    return (s->is_ref) ? 0 : s->refs;
} // nstr_refs

// 测试是否为字符串
bool nstr_is_str(nstr_p s)
{
    return (! s->is_ref);
} // nstr_is_str

// 测试是否为片段引用
bool nstr_is_ref(nstr_p s)
{
    return s->is_ref;
} // nstr_is_ref

// 校验完整性
bool nstr_verify(nstr_p s)
{
    return verify[s->encoding](s->buf, s->buf + s->bytes);
} // nstr_verify

inline static nstr_p new_slice(nstr_p s, void * begin, void * end, uint32_t chars, bool recount)
{
    uint32_t bytes = end - begin;
    nstr_p r = real_string(s);
    nstr_p n = calloc(1, nstr_object_size(bytes));

    if (! n) return NULL;

    n->src = r;
    n->offset = begin - r->buf;
    n->bytes = bytes;
    n->chars = (recount) ? count[encoding](begin, end) : chars;
    n->encoding = r->encoding;
    n->is_ref = 1;
    if (r != blank_strings[r->encoding]) {
        r->refs += 1; // 非空字符串需要增加引用计数。
    } // if
    return n;
} // new_slice

// 生成片段引用，或生成新字符串（字符范围）
nstr_p nstr_slice_chars(nstr_p s, bool no_ref, uint32_t index, uint32_t chars);
{
    void * begin = NULL;
    void * end = NULL;
    uint32_t ret_chars = 0;

    begin = real_buffer(r);
    end = begin + s->bytes;
    begin = locate[s->encoding](begin, end, index, &ret_chars);
    end = locate[s->encoding](begin, end, chars, &ret_chars);

    if (no_ref) return nstr_new(begin, end - begin, r->encoding);
    return new_slice(s, begin, end, ret_chars, false);
} // nstr_slice_chars

// 生成片段引用，或生成新字符串（字节范围）
nstr_p nstr_slice_bytes(nstr_p s, bool no_ref, void * pos, uint32_t bytes)
{
    if (no_ref) return nstr_new(pos, bytes, r->encoding);
    return new_slice(s, pos, pos + bytes, 0, true);
} // nstr_slice_bytes

typedef char_t * (*copy_strings_t)(char_t * pos, nstr_p * as, int n, char_t * dbuf, uint32_t dbytes);

static char_t * copy_strings(char_t * pos, nstr_p * as, int n, char_t * dbuf, uint32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
        do {
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 3:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 2:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 1:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        } while (b-- > 0);
        default: break;
    } // switch
    return pos;
} // copy_strings

static char_t * copy_strings_with_one_deli(char_t * pos, nstr_p * as, int n, char_t * dbuf, uint32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
        do {
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 3:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 2:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 1:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        } while (b-- > 0);
        default: break;
    } // switch
    return pos;
} // copy_strings_with_one_deli

static char_t * copy_strings_with_long_deli(char_t * pos, nstr_p * as, int n, char_t * dbuf, uint32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
        do {
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 3:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 2:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 1:
            memcpy(pos, real_buffer(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        } while (b-- > 0);
        default: break;
    } // switch
    return pos;
} // copy_strings_with_long_deli

static nstr_p join_strings(nstr_p deli, nstr_p as, int n, va_list * ap)
{
    va_list cp;
    copy_strings_t copy = &copy_strings;
    nstr_p new = NULL;
    nstr_p as2 = NULL;
    char_t * pos = NULL;
    char_t * dbuf = NULL;
    uint32_t bytes = 0;
    uint32_t chars = 0;
    uint32_t dbytes = 0;
    int i = 0;
    int n2 = 0;
    int cnt = 0;

    cnt += n;
    for (i = 0; i < n; ++i) {
        bytes += as[i]->bytes;
        chars += as[i]->chars;
    } // for

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        n2 = va_arg(cp, int);
        cnt += n2;
        for (i = 0; i < n2; ++i) {
            bytes += (as2[i])->bytes;
            chars += (as2[i])->chars;
        } // for
    } // while
    va_end(cp);

    if (cnt == 0) {
        return nstr_blank(as[0]->encoding);
    } // if

    if (deli) {
        dbuf = real_buffer(deli);
        dbytes = deli->bytes;

        bytes += dbytes * cnt; // 字节总数包含尾部间隔符，简化拷贝逻辑。
        chars += deli->chars * (cnt - 1); // 字符总数不包含尾部间隔符。

        if (deli->bytes == 1) {
            copy = &copy_strings_with_one_deli;
        } else {
            copy = &copy_strings_with_long_deli;
        } // if
    } // if

    new = calloc(1, nstr_object_size(bytes));
    if (! new) return NULL;

    pos = copy(new->buf, as, n, dbuf, dbytes);

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        pos = copy(pos, as2, va_args(cp, int), dbuf, dbytes);
    } // while
    va_end(cp);

    bytes -= dbytes; // 去掉多余的尾部间隔符。
    new->buf[bytes] = 0; // 设置终止 NUL 字符。
    init_string(new, STR_NEED_FREE, bytes, chars, as[0]->encoding);
    return new;
} // join_strings

// 合并多个字符串
nstr_p nstr_concat(nstr_p * as, int n, ...)
{
    va_list ap;
    nstr_p new = NULL;

    va_start(ap, n);
    new = join_strings(NULL, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_concat

// 使用间隔符，合并多个字符串
nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, ...)
{
    va_list ap;
    nstr_p new = NULL;

    va_start(ap, n);
    new = join_strings(deli, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_join

// 使用间隔符，分割字符串
nstr_p * nstr_split(nstr_p deli, nstr_p s, bool no_ref, int * max)
{
    nstr_p * as = NULL;
    nstr_p * an = NULL;
    void * pos = NULL;
    void * end = NULL;
    void * start = NULL;
    void * loc = NULL;
    uint32_t bytes = 0;
    int rmd = 0;
    int cnt = 0;
    int cap = 0;

    rmd = (max && *max > 0) ? *max : 0;
    cap = (rmd > 0) ? (rmd + 2) : 12; // max 次分割将产生 max + 1 个子串，再加上 1 个 NULL 终止标志。
    as = malloc(sizeof(as[0]) * cap);
    if (! as) return NULL;

    start = real_buffer(s);
    while ((loc = nstr_find(s, deli, &pos, &end, &bytes)) && --rmd > 0) {
        if (no_ref) {
            as[cnt] = nstr_new(start, loc - start, s->encoding);
        } else {
            as[cnt] = nstr_slice_bytes(s, start, loc - start);
        } // if
        start = loc + bytes;

        if (++cnt < cap - 2) continue; // 保留 2 个空槽给最后的子串和终止标志。
        an = realloc(as, sizeof(as[0]) * (cap + cap / 2));
        if (! an) {
            nstr_delete_all(as, cnt);
            return NULL;
        } // if
        cap = (cap + cap / 2);
        as = an;
        an = NULL;
    } // while

    if (no_ref) {
        as[cnt] = nstr_new(start, end - start, s->encoding);
    } else {
        as[cnt] = nstr_slice_bytes(s, start, end - start);
    } // if
    as[++cnt] = NULL;

    if (max) {
        *max = cnt;
    } // if
    return as;
} // nstr_split

// 搜索子字符串
void * nstr_find(nstr_p s, nstr_p sub, void ** pos, void ** end, uint32_t * bytes)
{
    void * loc = NULL;

    if (! *pos) {
        *pos = real_buffer(s);
        *end = *pos + s->bytes;
        *bytes = 0;
    } // if

    loc = memmem(*pos, *end - *pos, real_buffer(sub), sub->bytes);
    if (loc) {
        *pos = loc + sub->bytes;
        *bytes = sub->bytes;
    } else {
        *pos = NULL;
        *end = NULL;
        *bytes = 0;
    } // if
    return loc;
} // nstr_find

// 重新编码
nstr_p nstr_recode(nstr_p s, str_encoding_t encoding)
{
    if (s->encoding == encoding) {
        return s;
    } // if
    return NULL; // TODO
} // nstr_recode

// 初始化遍历字节
void * nstr_first_byte(nstr_p s, void ** pos, void ** end)
{
    *pos = real_buffer(s);
    *end = *pos + s->bytes;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        return NULL;
    } // if
    *pos += 1;
    return real_buffer(s);
} // nstr_first_byte

// 初始化遍历字符
void * nstr_first_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes)
{
    *pos = real_buffer(s);
    *end = *pos + s->bytes;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        *bytes = 0;
        return NULL;
    } // if
    *bytes = measure[s->encoding](*pos);
    *pos += *bytes;
    return real_buffer(s);
} // nstr_first_char

// 遍历字符
void * nstr_next_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes)
{
    void * loc = NULL;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        *bytes = 0;
        return NULL;
    } // if
    loc = *pos;
    *bytes = measure[s->encoding](*pos);
    *pos += *bytes;
    return loc;
} // nstr_next_char

// 返回空字符串
nstr_p nstr_blank(str_encoding_t encoding);
{
    return blank_strings[encoding];
} // nstr_blank
