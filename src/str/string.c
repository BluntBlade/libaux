#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "str/ascii.h"
#include "str/string.h"

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

typedef uint32_t (*measure_t)(void * pos);
static measure_t measure[NSTR_ENCODING_COUNT] = {
    &ascii_measure,
    &utf8_measure,
};

typedef void * (*check_t)(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes);
static check_t check[NSTR_ENCODING_COUNT] = {
    &ascii_check,
    &utf8_check,
};

typedef uint32_t (*count_t)(void * begin, void * end);
static count_t count[NSTR_ENCODING_COUNT] = {
    &ascii_count,
    &utf8_count,
};

typedef bool (*verify_t)(void * begin, void * end);
static verify_t verify[NSTR_ENCODING_COUNT] = {
    &ascii_verify,
    &utf8_verify,
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

inline static void init_slice(nstr_p s, bool need_free, nstr_p src, uint32_t offset, uint32_t bytes, uint32_t chars, uint32_t encoding)
{
    s->offset = offset;
    s->bytes = bytes;
    s->chars = chars;
    s->encoding = encoding;
    s->need_free = need_free ? 1 : 0;
    s->is_ref = 1;
    src->refs += 1;
} // init_slice

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

nstr_p nstr_clone(nstr_p s)
{
    return nstr_new(real_buffer(s), s->bytes, s->encoding);
} // nstr_clone

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

void nstr_delete_strings(nstr_p * as, int n)
{
    while (--n >= 0) nstr_delete(as[n]);
    free(as);
} // nstr_delete_strings

uint32_t nstr_encoding(nstr_p s)
{
    return s->encoding;
} // nstr_encoding

uint32_t nstr_bytes(nstr_p s)
{
    return s->bytes;
} // nstr_bytes

uint32_t nstr_chars(nstr_p s)
{
    return s->chars;
} // nstr_chars

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

uint32_t nstr_refs(nstr_p s)
{
    return (s->is_ref) ? 0 : s->refs;
} // nstr_refs

bool nstr_is_str(nstr_p s)
{
    return (! s->is_ref);
} // nstr_is_str

bool nstr_is_ref(nstr_p s)
{
    return s->is_ref;
} // nstr_is_ref

bool nstr_verify(nstr_p s)
{
    return verify[s->encoding](s->buf, s->buf + s->bytes);
} // nstr_verify

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

nstr_p nstr_blank(str_encoding_t encoding);
{
    nstr_p ret = blank_strings[encoding];
    ret.refs += 1;
    return ret;
} // nstr_blank

inline static nstr_p new_slice(nstr_p s, void * loc, uint32_t bytse, uint32_t chars)
{
    uint32_t bytes = end - begin;
    nstr_p r = real_string(s);
    nstr_p n = calloc(1, nstr_object_size(bytes));

    if (! n) return NULL;

    n->src = r;
    n->offset = begin - r->buf;
    n->bytes = bytes;
    n->chars = chars;
    n->encoding = r->encoding;
    n->is_ref = 1;

    r->refs += 1; // 增加引用计数。
    return n;
} // new_slice

nstr_p nstr_slice(nstr_p s, bool no_ref, uint32_t index, uint32_t chars);
{
    void * loc = NULL;
    uint32_t ret_chars = 0;
    uint32_t ret_bytes = 0;

    // CASE-1: 引用位置超出范围。
    // CASE-2: s 是空串。
    // CASE-3: 引用长度是零。
    // NOTE: 空串不会生成片段引用，直接返回空串本身。
    if (index >= s->chars || s->chars == 0 || chars == 0) return nstr_blank(s->encoding);

    // CASE-4: s 不是空串。
    begin = real_buffer(r);
    end = begin + s->bytes;

    ret_chars = s->chars;
    loc = check[s->encoding](begin, end, index, &ret_chars, &ret_bytes);
    if (! loc) return NULL;

    end = begin + ret_bytes;

    if (no_ref) return nstr_new(loc, ret_bytes, s->encoding);
    return new_slice(s, loc, ret_bytes, ret_chars);
} // nstr_slice

nstr_p nstr_slice_from(nstr_p s, bool no_ref, void * pos, uint32_t bytes)
{
    if (s->chars == 0) return nstr_blank(s->encoding); // CASE-1: s 是空串。
    if (no_ref) return nstr_new(pos, bytes, r->encoding);
    return new_slice(s, pos, bytes, count[encoding](begin, end));
} // nstr_slice_from

nstr_p * nstr_split(nstr_p deli, bool no_ref, nstr_p s, int * max);
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
        as[cnt] = nstr_slice_from(s, no_ref, start, loc - start);
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

    // 最后子串。
    as[cnt] = nstr_slice_from(s, no_ref, start, end - start);
    as[++cnt] = NULL;

    if (max) {
        *max = cnt;
    } // if
    return as;
} // nstr_split

typedef char_t * (*copy_strings_t)(char_t * pos, nstr_p * as, int n, char_t * dbuf, uint32_t dbytes);

static char_t * copy_strings(char_t * pos, nstr_p * as, int n, char_t * dbuf, uint32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
    while (b-- > 0) {
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
    } // while
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
    while (b-- > 0) {
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
    } // while
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
    while (b-- > 0) {
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
    } // while
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

nstr_p nstr_repeat(nstr_p s, int n)
{
    nstr_p as[16] = {
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
    };
    nstr_p new = NULL;
    uint32_t b = 0;

    if (s->bytes == 0) return nstr_blank(s->encoding); // CASE-1: s 是空串。
    if (n <= 1) {
        real_string(s)->refs += 1;
        return s;
    } // if

    new = calloc(1, nstr_object_size(s->bytes * n));
    if (! new) return NULL;

    pos = copy_strings(new->buf, as, n % (sizeof(as) / sizeof(as[0])), NULL, 0);

    b = n / (sizeof(as) / sizeof(as[0]));
    while (b-- > 0) pos = copy_strings(pos, as, (sizeof(as) / sizeof(as[0])), NULL, 0);

    init_string(new, STR_NEED_FREE, s->bytes * n, s->chars * n, s->encoding);
    return new;
} // repeat

nstr_p nstr_concat(nstr_p * as, int n, ...)
{
    va_list ap;
    nstr_p new = NULL;

    va_start(ap, n);
    new = join_strings(NULL, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_concat

nstr_p nstr_concat2(nstr_p s1, nstr_p s2)
{
    nstr_p new = NULL;
    uint32_t bytes = 0;

    bytes = s1->bytes + s2->bytes;
    if (bytes == 0) return nstr_blank(s1->encoding);

    new = calloc(1, nstr_object_size(bytes));
    if (! new) return NULL;

    memcpy(new->buf, real_buffer(s1), s1->bytes);
    memcpy(new->buf + s1->bytes, real_buffer(s2), s2->bytes);
    init_string(new, STR_NEED_FREE, bytes, s1->chars + s2->chars, s1->encoding);
    return new;
} // nstr_concat2

nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3)
{
    nstr_p new = NULL;
    uint32_t bytes = 0;

    bytes = s1->bytes + s2->bytes + s3->bytes;
    if (bytes == 0) return nstr_blank(s1->encoding);

    new = calloc(1, nstr_object_size(bytes));
    if (! new) return NULL;

    memcpy(new->buf, real_buffer(s1), s1->bytes);
    memcpy(new->buf + s1->bytes, real_buffer(s2), s2->bytes);
    memcpy(new->buf + s1->bytes + s2->bytes, real_buffer(s3), s3->bytes);
    init_string(new, STR_NEED_FREE, bytes, s1->chars + s2->chars + s3->chars, s1->encoding);
    return new;
} // nstr_concat3

nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, ...)
{
    va_list ap;
    nstr_p new = NULL;

    va_start(ap, n);
    new = join_strings(deli, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_join

nstr_p nstr_join_with_char(char_t deli, nstr_p * as, int n, ...)
{
    va_list ap;
    nstr_t d = {0};
    nstr_p new = NULL;

    d->buf[0] = deli;
    init_string(&d, STR_NEED_FREE, 1, 1, STR_ASCII);

    va_start(ap, n);
    new = join_strings(d, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_join_with_char

nstr_p nstr_replace(nstr_p s, bool no_ref, uint32_t index, uint32_t chars, nstr_p sub)
{
    nstr_t s1 = {0};
    nstr_t s3 = {0};
    void * begin = NULL;
    void * loc = NULL;
    void * end = NULL;
    uint32_t ret_chars = 0;
    uint32_t ret_bytes = 0;

    if (s->chars == 0) return nstr_slice_from(sub, no_ref, real_buffer(sub), sub->bytes); // CASE-1: s 是空串。
    if (sub->chars == 0) return nstr_slice_from(s, no_ref, real_buffer(s), s->bytes); // CASE-2: sub 是空串。

    if (index >= s->chars) return nstr_concat2(s, sub); // CASE-3: 替换位置在串尾。
    if (index == 0 && chars == 0) return nstr_concat2(sub, s); // CASE-4: 替换位置在串头且替换长度为零。

    // CASE-5: 替换位置在串中且替换长度不为零。
    begin = real_buffer(s);
    end = s + s->bytes;

    ret_chars = chars;
    loc = check[s->encoding](begin, end, index, &ret_chars, &ret_bytes);
    if (! loc) return NULL;

    init_slice(&s1, STR_DONT_FREE, s, 0, loc - begin, index, s->encoding);
    init_slice(&s3, STR_DONT_FREE, s, loc - begin + ret_bytes, ret_bytes, ret_chars, s->encoding);
    return nstr_concat3(s1, sub, s2);
} // nstr_replace

nstr_p nstr_replace_char(nstr_p s, bool no_ref, uint32_t index, uint32_t chars, char_t ch)
{
    nstr_t sub = {0};

    sub->buf[0] = ch;
    init_string(sub, STR_DONT_FREE, 1, 1, s->encoding);
    return nstr_replace(s, no_ref, index, chars, &sub);
} // nstr_replace_char

nstr_p nstr_remove(nstr_p s, bool no_ref, uint32_t index, uint32_t chars)
{
    return nstr_replace(s, no_ref, index, chars, blank_strings[encoding]);
} // nstr_remove

nstr_p nstr_cut_head(nstr_p s, bool no_ref, uint32_t chars)
{
    if (s->chars < chars) return nstr_blank(s->encoding); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, no_ref, 0, chars, blank_strings[encoding]);
} // nstr_cut_head

nstr_p nstr_cut_tail(nstr_p s, bool no_ref, uint32_t chars)
{
    if (s->chars < chars) return nstr_blank(s->encoding); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, no_ref, s->chars - chars, chars, blank_strings[encoding]);
} // nstr_cut_tail
