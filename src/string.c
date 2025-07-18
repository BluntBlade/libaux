#include <assert.h>
#include <string.h>

#include "nstr.h"

typedef uint8_t char_t;

typedef struct NSTR {
    uint64_t is_ref:1;          // 片段标志位：0 表示字符串，1 表示片段引用。
    uint64_t encoding:9;        // 编码方案代号，最多支持 512 种编码。
    uint64_t chars:18;          // 按编码方案解释后的字符个数。
    uint64_t bytes:18;          // 字符串字节数。
    union {
        uint64_t refs:18;       // 片段引用计数，生成字符串时置 1 ，表示对自身的引用。减到 0 时释放内存。
        uint64_t offset:18;     // 片段在源字符串内存区的起始位置（相对于起点的字节数）。
    };

    union {
        char_t        buf[1];   // 单字节字符数据内存区，包含结尾的 NUL 字符。
        struct NSTR * src;      // 指向被引用的字符串，其 buf 与 offset 相加得到片段所在内存起点位置。
    };
} nstr_t;

// ---- ASCII related functions ---- //

static uint64_t measure_ascii(void * pos)
{
    return 1;
} // measure_ascii

static void * locate_ascii(void * begin, void * end, uint64_t index, uint64_t * chars)
{
    uint64_t bytes = end - begin;
    if (index >= bytes) {
        *chars = 0;
        return end;
    } // if
    *chars = index;
    return begin + index;
} // locate_ascii

static uint64_t count_ascii(void * begin, void * end)
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

static uint64_t measure_utf8(void * pos)
{
    char_t ch = ((char_t *)pos)[0];
    if ((ch & 0x80) == 0) return 1;
    if (((ch <<= 1) & 0x80) == 0) return 0;
    if (((ch <<= 1) & 0x80) == 0) return 2;
    if (((ch <<= 1) & 0x80) == 0) return 3;
    if (((ch <<= 1) & 0x80) == 0) return 4;
    return 0;
} // measure_utf8

static void * locate_utf8(void * begin, void * end, uint64_t index, uint64_t * chars)
{
    uint64_t cnt = 0;
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

static uint64_t count_utf8(void * begin, void * end)
{
    uint64_t cnt = 0;
    locate_utf8(begin, end, 0, &cnt);
    return cnt;
} // count_utf8

static bool verify_utf8(void * begin, void * end)
{
    uint64_t bytes = 0;
    void * pos = begin;
    while (pos < end) {
        bytes = measure_utf8(pos);
        if (bytes == 0) return false;
        pos += bytes;
    } // while
    return true;
} // verify_utf8

typedef uint64_t (*measure_t)(void * pos);
static measure_t measure[NSTR_ENCODING_COUNT] = {
    &measure_ascii,
    &measure_utf8,
};

typedef void * (*locate_t)(void * begin, void * end, uint64_t index, uint64_t * chars);
static locate_t locate[NSTR_ENCODING_COUNT] = {
    &locate_ascii,
    &locate_utf8,
};

typedef uint64_t (*count_t)(void * begin, void * end);
static count_t count[NSTR_ENCODING_COUNT] = {
    &count_ascii,
    &count_utf8,
};

typedef bool (*verify_t)(void * begin, void * end);
static verify_t verify[NSTR_ENCODING_COUNT] = {
    &verify_ascii,
    &verify_utf8,
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
size_t nstr_object_bytes(uint64_t bytes)
{
    return sizeof(nstr_t) + bytes;
} // nstr_object_bytes

// 从原生字符串生成新字符串
nstr_p nstr_new(void * src, uint64_t bytes, str_encoding_t encoding)
{
    nstr_p s = NULL;
    
    assert(verify[encoding](src, src + bytes));

    if ((s = calloc(1, nstr_object_bytes(bytes)))) {
        memcpy(s->buf, src, bytes);
        s->refs = 1;  // 引用自身。
        s->bytes = bytes;
        s->chars = count[encoding](src, src + bytes);
        s->encoding = encoding;
        s->is_ref = 0;
    } // if
    return s;
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

    s->refs -= 1;
    if (s->refs == 0) {
        free(s); // 释放字符串
    } // if

    *ps = NULL; // 防止野指针
} // nstr_delete

// 删除切分后的字符串数组
void nstr_delete_all(nstr_p * as, int n)
{
    while (--n >= 0) nstr_delete(as[n]);
    free(as);
} // nstr_delete_all

// 返回编码方案代号
uint64_t nstr_encoding(nstr_p s)
{
    return s->encoding;
} // nstr_encoding

// 返回字节数，不包含最后的 NUL 字符
uint64_t nstr_bytes(nstr_p s)
{
    return s->bytes;
} // nstr_bytes

// 返回字符数，不包含最后的 NUL 字符
uint64_t nstr_chars(nstr_p s)
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
uint64_t nstr_refs(nstr_p s)
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

inline static nstr_p new_slice(nstr_p s, void * begin, void * end, uint64_t chars, bool recount)
{
    uint64_t bytes = end - begin;
    nstr_p r = real_string(s);
    nstr_p n = calloc(1, nstr_object_bytes(bytes));

    if (! n) return NULL;

    n->src = r;
    n->offset = begin - r->buf;
    n->bytes = bytes;
    n->chars = (recount) ? count[encoding](begin, end) : chars;
    n->encoding = r->encoding;
    n->is_ref = 1;
    r->refs += 1; // 增加引用
    return n;
} // new_slice

// 生成片段引用，或生成新字符串（字符范围）
nstr_p nstr_slice_chars(nstr_p s, bool no_ref, uint64_t index, uint64_t chars);
{
    void * begin = NULL;
    void * end = NULL;
    uint64_t ret_chars = 0;

    begin = real_buffer(r);
    end = begin + s->bytes;
    begin = locate[s->encoding](begin, end, index, &ret_chars);
    end = locate[s->encoding](begin, end, chars, &ret_chars);

    if (no_ref) return nstr_new(begin, end - begin, r->encoding);
    return new_slice(s, begin, end, ret_chars, false);
} // nstr_slice_chars

// 生成片段引用，或生成新字符串（字节范围）
nstr_p nstr_slice_bytes(nstr_p s, bool no_ref, void * pos, uint64_t bytes)
{
    if (no_ref) return nstr_new(pos, bytes, r->encoding);
    return new_slice(s, pos, pos + bytes, 0, true);
} // nstr_slice_bytes

// 合并多个字符串
extern nstr_p nstr_concat(nstr_p * as, int n, ...);

// 使用间隔符，合并多个字符串
extern nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, ...);

// 使用间隔符，分割字符串
nstr_p * nstr_split(nstr_p deli, nstr_p s, bool no_ref, int * max)
{
    nstr_p * as = NULL;
    nstr_p * an = NULL;
    void * pos = NULL;
    void * end = NULL;
    void * start = NULL;
    void * loc = NULL;
    uint64_t bytes = 0;
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
void * nstr_find(nstr_p s, nstr_p sub, void ** pos, void ** end, uint64_t * bytes)
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

// 初始化遍历
bool nstr_first(nstr_p s, bool by_char, void ** pos, void ** end, uint64_t * bytes)
{
    *pos = real_buffer(s);
    *end = *pos + s->bytes;
    if (*pos == *end) {
        *bytes = 0;
        return false;
    } // if
    bytes = by_char ? measure[s->encoding](*pos) : 1;
    return true;
} // nstr_first

// 遍历字符
bool nstr_next_char(nstr_p s, void ** pos, void ** end, uint64_t * bytes)
{
    *pos += *bytes;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        return false;
    } // if
    *bytes = measure[s->encoding](*pos);
    return true;
} // nstr_next_char
