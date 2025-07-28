#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "str/ascii.h"
#include "str/string.h"

typedef uint32_t (*measure_t)(void * pos);
typedef void * (*check_t)(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes);
typedef uint32_t (*count_t)(void * begin, void * end);
typedef bool (*verify_t)(void * begin, void * end);

typedef struct NSTR_VTABLE {
    measure_t   measure;        // 度量单个字符的字节数
    check_t     check;          // 检查一段字节是否包含正确编码的字符
    count_t     count;          // 计算一段字节包含的字符数
    verify_t    verify;         // 校验一段字节是否包含正确编码的字符
} nstr_vtable_t, *nstr_vtable_p;

typedef struct NSTR {
    uint32_t        is_slice:1;     // 类型标志位：0 表示字符串，1 表示切片
    uint32_t        bytes:31;       // 串内容占用字节数
    uint32_t        need_free:1;    // 是否释放内存：0 表示不需要，1 表示需要
    uint32_t        chars:31;       // 编码后的字符个数

    union {
        struct {
            nstr_vtable_p   vtbl;           // 虚拟函数表
            uint32_t        unused:1;
            uint32_t        refs:31;        // 引用计数，生成字符串时置 1 ，表示对自身的引用。减到 0 时释放内存
            char_t          buf[4];         // 单字节字符数据内存区，包含结尾的 NUL 字符
        } str;
        struct {
            uint32_t        unused1:1;
            uint32_t        offset:31;      // 切片首字节在源串的偏移位置（单位：字节）
            uint32_t        unused2:1;
            uint32_t        index:31;       // 切片首字符在源串的位置（单位：字符）
            struct NSTR *   ent;            // 指向被引用的字符串实体，其 buf 与 offset 相加得到切片所在内存起点位置
        } slc;
    };
} nstr_t;

nstr_vtable_t vtable[NSTR_ENCODING_COUNT] = {
    {
        &ascii_measure,
        &ascii_check,
        &ascii_count,
        &ascii_verify,
        &ascii_blank,
    },
    {
        &utf8_measure,
        &utf8_check,
        &utf8_count,
        &utf8_verify,
        &utf8_blank,
    },
};

inline static void * get_start(nstr_p s)
{
    return (s->is_slice) ? s->slc->ent->str.buf + s->slc.offset : s->str.buf;
} // get_start

inline static nstr_p get_entity(nstr_p s)
{
    return (s->is_slice) ? s->slc->ent : s;
} // get_entity

inline static nstr_p get_vtable(nstr_p s)
{
    return (s->is_slice) ? s->slc->ent->str.vtbl : s->str.vtbl;
} // get_entity

// 字符串对象占用字节数
inline static size_t entity_size(uint32_t bytes)
{
    return sizeof(nstr_t) + bytes;
} // entity_size

// 初始化字符串对象
inline static void init_entity(nstr_p s, bool need_free, uint32_t bytes, uint32_t chars, nstr_vtable_p vtbl)
{
    s->is_slice = 0;
    s->need_free = need_free ? 1 : 0;
    s->bytes = bytes;
    s->chars = chars;
    s->str.refs = 1;  // 引用自身。
    s->str.vtbl = vtbl;
} // init_entity

size_t nstr_slice_size(void)
{
    return sizeof(nstr_t);
} // nstr_slice_size

void nstr_init_slice(nstr_p s, bool need_free, nstr_p src, uint32_t offset, uint32_t bytes, uint32_t index, uint32_t chars)
{
    s->is_slice = 1;
    s->need_free = need_free ? 1 : 0;
    s->bytes = bytes;
    s->chars = chars;
    s->slc.ent = src;
    s->slc.offset = offset;
    s->slc.index = index;
    nstr_add_ref(src);
} // nstr_init_slice

nstr_p nstr_new(void * src, uint32_t bytes, str_encoding_t encoding)
{
    nstr_p new = NULL;
    nstr_vtable_p vtbl = &vtable[encoding];

    if (bytes == 0) return nstr_blank(encoding);
    if ((new = calloc(1, entity_size(bytes)))) {
        memcpy(new->str.buf, src, bytes);
        init_entity(new, STR_NEED_FREE, bytes, vtbl->count(src, src + bytes), vtbl);
    } // if
    return new;
} // nstr_new

nstr_p nstr_clone(nstr_p s)
{
    return nstr_new(get_start(s), s->bytes, nstr_encoding(s));
} // nstr_clone

void nstr_delete(nstr_p * ps)
{
    nstr_p ent = *ps;

    if (! ent) return;
    if (ent->is_slice) {
        ent = (*ps)->ent;
        if ((*ps)->need_free) free(*ps); // 释放切片
    } // if

    *ps = NULL; // 防止野指针

    ent->str.refs -= 1;
    if (ent->str.refs == 0 && ent->need_free) free(ent); // 释放非空字符串
} // nstr_delete

void nstr_delete_strings(nstr_p * as, int n)
{
    if (as) {
        while (--n >= 0) nstr_delete(as[n]);
        free(as);
    } // if
} // nstr_delete_strings

nstr_p nstr_add_ref(nstr_p s)
{
    get_entity(s)->str.refs += 1;
    return s;
} // nstr_add_ref

uint32_t nstr_encoding(nstr_p s)
{
    return ((void *)get_vtable(s) - (void *)&vtable) / sizeof(nstr_vtable_t);
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
    nstr_p new = NULL;
    nstr_p s = *ps;

    if (s->is_slice) {
        new = nstr_clone(*ps);
        if (! new) return NULL;
        *ps = new;
    } // if
    return (*ps)->str.buf;
} // nstr_to_cstr

uint32_t nstr_refs(nstr_p s)
{
    return (s->is_slice) ? 0 : s->str.refs;
} // nstr_refs

bool nstr_is_string(nstr_p s)
{
    return (! s->is_slice);
} // nstr_is_string

bool nstr_is_slice(nstr_p s)
{
    return s->is_slice;
} // nstr_is_slice

bool nstr_verify(nstr_p s)
{
    return get_vtable(s)->verify(get_start(s), get_start(s) + s->bytes);
} // nstr_verify

void * nstr_first_byte(nstr_p s, void ** pos, void ** end)
{
    *pos = get_start(s);
    *end = *pos + s->bytes;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        return NULL;
    } // if
    *pos += 1;
    return get_start(s);
} // nstr_first_byte

void * nstr_first_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes)
{
    *pos = get_start(s);
    *end = *pos + s->bytes;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        *bytes = 0;
        return NULL;
    } // if
    *bytes = get_vtable(s)->measure(*pos);
    *pos += *bytes;
    return get_start(s);
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
    *bytes = get_vtables(s)->measure(*pos);
    *pos += *bytes;
    return loc;
} // nstr_next_char

void * nstr_first_sub(nstr_p s, nstr_p sub, void ** start, uint32_t * size, uint32_t * index)
{
    void * loc = NULL;

    if (sub->chars == 0) {
        // 子串为空
        loc = get_start(s);
        goto NSTR_FIRST_SUB_END;
    } // if
    if (s->chars == 0) goto NSTR_FIRST_SUB_END; // 源串为空，找不到子串

    if (! *start) {
        *start = get_start(s);
        *size = s->bytes;
    } // if
    if (*size < sub->bytes) goto NSTR_FIRST_SUB_END; // 查找范围小于子串长度

    loc = nstr_next_sub(s, pub, start, size, index);
    if (loc) {
        *index -= sub->chars;
        return loc;
    } // if

NSTR_FIRST_SUB_END:
    *start = NULL;
    *size = 0;
    *index = 0;
    return loc;
} // nstr_first_sub

void * nstr_next_sub(nstr_p s, nstr_p sub, void ** start, uint32_t * size, uint32_t * index)
{
    void * loc = NULL;
    
    if (! *start || *size < sub->bytes) goto NSTR_NEXT_SUB_END; // 查找范围耗尽

    if (sub->bytes == 1) {
        // 子串只有一个字节长
        loc = memchr(*start, get_start(sub)[0], *size);
    } else {
        loc = memmem(*start, *size, get_start(sub), sub->bytes);
    } // if
    if (! loc) goto NSTR_NEXT_SUB_END; // 找不到子串

    *index += sub->chars + get_vtable(s)->count(start, loc);

    *size -= ((loc - *start) + sub->bytes); // 缩小字节范围
    *start = loc + sub->bytes; // 移到下一个起点
    return loc;

NSTR_NEXT_SUB_END:
    *start = NULL;
    *size = 0;
    *index = 0;
    return loc;
} // nstr_next_sub

nstr_p nstr_blank(str_encoding_t encoding);
{
    return nstr_add_ref(blank_strings[encoding]);
} // nstr_blank

inline static nstr_p new_slice(nstr_p s, void * loc, uint32_t bytes, uint32_t chars)
{
    nstr_p ent = get_entity(s);
    nstr_p new = calloc(1, entity_size(bytes));
    if (! new) return NULL;
    nstr_init_slice(new, STR_NEED_FREE, ent, loc - ent->str.buf, bytes, chars);
    return nstr_add_ref(s); // 增加引用计数。
} // new_slice

nstr_p nstr_slice(nstr_p s, bool can_new, uint32_t index, uint32_t chars);
{
    void * loc = NULL;
    uint32_t ret_chars = 0;
    uint32_t ret_bytes = 0;

    // CASE-1: 切片起点超出范围
    // CASE-2: 源串是空串
    // CASE-3: 切片长度是零
    // NOTE: 不生成切片，直接返回空串
    if (index >= s->chars || s->chars == 0 || chars == 0) return nstr_blank(nstr_encoding(s));

    // CASE-4: 源字符串不是空串
    ret_chars = s->chars - index; // 最大切片范围是整个源串
    loc = get_start(s);
    loc = get_vtable(s)->check(loc, loc + s->bytes, index, &ret_chars, &ret_bytes);
    if (! loc) return NULL; // 源串包含异常字节

    if (can_new) return nstr_new(loc, ret_bytes, nstr_encoding(s));
    return new_slice(s, loc, ret_bytes, ret_chars);
} // nstr_slice

nstr_p nstr_slice_from(nstr_p s, bool can_new, void * pos, uint32_t bytes)
{
    if (s->chars == 0) return nstr_blank(nstr_encoding(s)); // CASE-1: 源串是空串
    if (can_new) return nstr_new(pos, bytes, nstr_encoding(s));
    return new_slice(s, pos, bytes, get_vtable(s)->count(pos, pos + bytes));
} // nstr_slice_from

inline static bool augment_array(nstr_p ** as, int * cap, int delta)
{
    nstr_p * an = realloc(*as, sizeof((*as)[0]) * (*cap + delta)); // 数组扩容
    if (! an) return false;
    *cap += delta;
    *as = an;
    return true;
} // augment_array

static void * next_char_to_split(nstr_p s, nstr_p sub, void ** start, uint32_t * size, uint32_t * index)
{
    uint32_t ret_bytes = 0;
    uint32_t ret_chars = 1;
    *start = get_vtable->check(*start, *start + *size, 0, &ret_chars, &ret_bytes);
    *size -= ret_bytes;
    return *start;
} // next_char_to_split

nstr_p * nstr_split(nstr_p s, bool can_new, nstr_p deli, int * max);
{
    static void * (*next[2])(nstr_p, nstr_p, void **, uint32_t *, uint32_t *) = {
        &next_char_to_split, // 将每个字符切分为一个子串
        &nstr_next_sub, // 寻找分隔符并切分相邻子串
    };

    nstr_p * as = NULL; // 子串数组
    void * pos = NULL; // 子串起点
    void * loc = NULL; // 分隔符起点
    void * start = NULL; // 查找范围起点
    void * end = NULL; // 查找范围终点
    uint32_t size = 0; // 查找范围字节数
    uint32_t index = 0; // 分隔符首字符的索引
    int rmd = 0; // 剩余切分次数，零表示停止，负数表示无限次
    int cnt = 0; // 子串数量，用于下标时始终指向下一个可用元素
    int cap = 0; // 数组容量
    int delta = 0; // 减量
    int sel = 0; // 选择子

    if (s->chars == 0) {
        // CASE-1: 源串是空串
        as = malloc(sizeof(as[0]) * 2);
        if (! as) return NULL;
        as[0] = nstr_blank(nstr_encoding(s));
        as[1] = NULL;
        if (max) *max = 1;
        return as;
    } // if

    rmd = (max && *max > 0) ? *max : -1;
    delta = (rmd > 0) ? 1 : 0;

    // max 次分割将产生 max + 1 个子串，再加上 1 个 NULL 终止标志
    // 保留 2 个元素给最后的子串和终止标志
    cap = (rmd < 16 - 2) ? 16 : (rmd + 2);

    as = malloc(sizeof(as[0]) * cap);
    if (! as) goto NSTR_SPLIT_END;

    pos = get_start(s);
    start = pos;
    size = s->bytes;
    end = start + size;
    sel = (deli && deli->chars > 0) 1 : 0;
    while (rmd != 0) {
        loc = next[sel](s, deli, &start, &size, &index);
        if (! loc) {
            // 源串包含异常字节
            nstr_delete_all(as, cnt);
            return NULL;
        } // if
        if (loc == end) break; // 找不到切分位置

        as[cnt++] = nstr_slice_from(s, can_new, pos, loc - pos);

        if (cnt >= cap - 2 && ! augment_array(&as, &cap, 16)) goto NSTR_SPLIT_END;
        rmd -= delta;
        pos = start; // 移到下一个子串起点
    } // while

    as[cnt++] = nstr_slice_from(s, can_new, pos, s->bytes - (pos - get_start(s))); // 最后子串。
    as[cnt] = NULL; // 设置终止标志

NSTR_SPLIT_END:
    if (max) *max = cnt;
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
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 3:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 2:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 1:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
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
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 3:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 2:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 1:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
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
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 3:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 2:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 1:
            memcpy(pos, get_start(as[i]), (as[i])->bytes);
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
        return nstr_blank(nstr_encoding(as[0]));
    } // if

    if (deli) {
        dbuf = get_start(deli);
        dbytes = deli->bytes;

        bytes += dbytes * cnt; // 字节总数包含尾部间隔符，简化拷贝逻辑。
        chars += deli->chars * (cnt - 1); // 字符总数不包含尾部间隔符。

        if (deli->bytes == 1) {
            copy = &copy_strings_with_one_deli;
        } else {
            copy = &copy_strings_with_long_deli;
        } // if
    } // if

    new = calloc(1, entity_size(bytes));
    if (! new) return NULL;

    pos = copy(new->str.buf, as, n, dbuf, dbytes);

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        pos = copy(pos, as2, va_args(cp, int), dbuf, dbytes);
    } // while
    va_end(cp);

    bytes -= dbytes; // 去掉多余的尾部间隔符。
    new->str.buf[bytes] = 0; // 设置终止 NUL 字符。
    init_entity(new, STR_NEED_FREE, bytes, chars, nstr_encoding(as[0]));
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

    if (s->bytes == 0) return nstr_blank(nstr_encoding(s)); // CASE-1: s 是空串。
    if (n <= 1) return nstr_add_ref(s);

    new = calloc(1, entity_size(s->bytes * n));
    if (! new) return NULL;

    pos = copy_strings(new->str.buf, as, n % (sizeof(as) / sizeof(as[0])), NULL, 0);

    b = n / (sizeof(as) / sizeof(as[0]));
    while (b-- > 0) pos = copy_strings(pos, as, (sizeof(as) / sizeof(as[0])), NULL, 0);

    init_entity(new, STR_NEED_FREE, s->bytes * n, s->chars * n, get_vtable(s));
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
    if (bytes == 0) return nstr_blank(nstr_encoding(s1));

    new = calloc(1, entity_size(bytes));
    if (! new) return NULL;

    memcpy(new->str.buf, get_start(s1), s1->bytes);
    memcpy(new->str.buf + s1->bytes, get_start(s2), s2->bytes);
    init_entity(new, STR_NEED_FREE, bytes, s1->chars + s2->chars, nstr_encoding(s1));
    return new;
} // nstr_concat2

nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3)
{
    nstr_p new = NULL;
    uint32_t bytes = 0;

    bytes = s1->bytes + s2->bytes + s3->bytes;
    if (bytes == 0) return nstr_blank(nstr_encoding(s1));

    new = calloc(1, entity_size(bytes));
    if (! new) return NULL;

    memcpy(new->str.buf, get_start(s1), s1->bytes);
    memcpy(new->str.buf + s1->bytes, get_start(s2), s2->bytes);
    memcpy(new->str.buf + s1->bytes + s2->bytes, get_start(s3), s3->bytes);
    init_entity(new, STR_NEED_FREE, bytes, s1->chars + s2->chars + s3->chars, nstr_encoding(s1));
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

    d->str.buf[0] = deli;
    init_entity(&d, STR_NEED_FREE, 1, 1, &vtable[STR_ENC_ASCII]);

    va_start(ap, n);
    new = join_strings(d, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_join_with_char

nstr_p nstr_replace(nstr_p s, bool can_new, uint32_t index, uint32_t chars, nstr_p sub)
{
    nstr_t s1 = {0};
    nstr_t s3 = {0};
    nstr_p ent = NULL; // 最终字符串对象
    void * begin = NULL;
    void * mid = NULL;
    uint32_t offset = 0;
    uint32_t mid_chars = 0;
    uint32_t mid_bytes = 0;

    if (s->chars == 0) return nstr_slice_from(sub, can_new, get_start(sub), sub->bytes); // CASE-1: s 是空串。
    if (sub->chars == 0) return nstr_slice_from(s, can_new, get_start(s), s->bytes); // CASE-2: sub 是空串。

    if (index >= s->chars) return nstr_concat2(s, sub); // CASE-3: 替换位置在串尾。
    if (index == 0 && chars == 0) return nstr_concat2(sub, s); // CASE-4: 替换位置在串头且替换长度为零。

    // CASE-5: 待替换部分在源串中间且长度不为零。
    if (s->is_slice) {
        ent = s->ent;
        offset = s->slc.offset;
    } else {
        ent = s;
        offset = 0;
    } // if

    mid_chars = s->chars;
    begin = ent->str.buf + offset;
    mid = get_vtable(s)->check(begin, begin + s->bytes, index, &mid_chars, &mid_bytes);
    if (! mid) return NULL; // 源串包含异常字节

    s1_bytes = mid - begin;
    nstr_init_slice(&s1, STR_DONT_FREE, ent, offset, s1_bytes, index);
    nstr_init_slice(&s3, STR_DONT_FREE, ent, offset + s1_bytes, s->bytes - s1_bytes - mid_bytes, s->chars - index - mid_chars);
    return nstr_concat3(s1, sub, s2);
} // nstr_replace

nstr_p nstr_replace_char(nstr_p s, bool can_new, uint32_t index, uint32_t chars, char_t ch)
{
    nstr_t sub = {0};

    sub->str.buf[0] = ch;
    init_entity(sub, STR_DONT_FREE, 1, 1, get_vtable(s));
    return nstr_replace(s, can_new, index, chars, &sub);
} // nstr_replace_char

nstr_p nstr_remove(nstr_p s, bool can_new, uint32_t index, uint32_t chars)
{
    return nstr_replace(s, can_new, index, chars, nstr_blank(nstr_encoding(s)));
} // nstr_remove

nstr_p nstr_cut_head(nstr_p s, bool can_new, uint32_t chars)
{
    if (s->chars < chars) return nstr_blank(nstr_encoding(s)); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, can_new, 0, chars, nstr_blank(nstr_encoding(s))); // TODO: No need to add reference of blank string
} // nstr_cut_head

nstr_p nstr_cut_tail(nstr_p s, bool can_new, uint32_t chars)
{
    if (s->chars < chars) return nstr_blank(nstr_encoding(s)); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, can_new, s->chars - chars, chars, nstr_blank(nstr_encoding(s))); // TODO: No need to add reference of blank string
} // nstr_cut_tail
