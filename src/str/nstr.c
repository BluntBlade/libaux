#include <stdarg.h>
#include <stdlib.h>

#include "str/ascii.h"
#include "str/utf8.h"
#include "str/nstr.h"

#define container_of(type, member, addr) ((type *)((void *)(addr) - (void *)(&(((type *)0)->member))))

typedef int32_t (*measure_t)(const char_t * pos);
typedef int32_t (*count_t)(const char_t * start, int32_t size, int32_t * chars);

typedef struct VTABLE {
    measure_t   measure;        // 度量单个字符的字节数
    count_t     count;          // 计算字节范围包含的字符数
} vtable_t, *vtable_p;

typedef struct ENTITY {
    int32_t         bytes;          // 串内容占用字节数

    uint32_t        need_free:1;    // 是否释放内存
    uint32_t        unused:31;

    int32_t         slcs;           // （仅用于字符串）切片计数，减到 0 则销毁字符串并释放内存
    char_t          data[1];        // 字符存储区，包含结尾的 NUL 字符
} entity_t, *entity_p;

typedef struct NSTR {
    int32_t         bytes;          // 串内容占用字节数
    int32_t         chars;          // 编码后的字符个数

    uint32_t        need_free:1;    // 是否释放内存
    uint32_t        unused:25;
    uint32_t        encoding:6;     // 编码方案，支持最多 64 种

    int32_t         offset;         // （仅用于切片）字符数据起始地址相对于源地址的偏移量，>= 0 表示引用 NSTR 字符串，< 0 表示引用 C 字符串

    const char_t *  start;          // 字符数据起始地址
} nstr_t;

// ---- 静态变量 ---- //

vtable_t vtable[STR_ENC_COUNT] = {
    {
        &ascii_measure,
        &ascii_count,
    },
    {
        &utf8_measure,
        &utf8_count,
    },
};

entity_t cstr_ent = {0};
entity_t blank_ent = {0};

inline static entity_p get_entity(nstr_p s)
{
    return (s->offset < 0) ? &cstr_ent : container_of(entity_t, data, (s->start - s->offset));
} // get_entity

inline static void add_ref(entity_p ent)
{
    ent->slcs += 1;
} // add_ref

inline static void del_ref(entity_p ent)
{
    if (--ent->slcs == 0 && ent->need_free) free(ent);
} // del_ref

static entity_p new_entity(int32_t bytes)
{
    entity_p new = malloc(sizeof(entity_t) + bytes);
    if (new) {
        new->need_free = true;
        new->bytes = bytes;
        new->slcs = 1;  // 引用自身
    } // if
    return new;
} // new_entity

inline static nstr_p init_slice(nstr_p s, bool need_free, const char_t * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    s->need_free = need_free;
    s->encoding = encoding;
    s->bytes = bytes;
    s->chars = chars;
    s->offset = offset;
    s->start = start;

    add_ref(get_entity(s));
    return s;
} // init_slice

static nstr_p new_slice(const char_t * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    nstr_p new = malloc(sizeof(nstr_t));
    if (new) init_slice(new, true, start, offset, bytes, chars, encoding);
    return new;
} // new_slice

inline static nstr_p refer_to_other(nstr_p r, const char_t * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    del_ref(get_entity(r));
    r->bytes = bytes;
    r->chars = chars;
    r->offset = offset;
    r->start = start;
    r->encoding = encoding;
    add_ref(get_entity(r));
    return r;
} // refer_to_other

inline static nstr_p refer_to_or_new_slice(nstr_p r, const char_t * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    if (r) return refer_to_other(r, start, offset, bytes, chars, encoding);
    return new_slice(start, offset, bytes, chars, encoding);
} // refer_to_or_new_slice

inline static nstr_p refer_to_whole(nstr_p r, nstr_p s)
{
    return refer_to_or_new_slice(r, s->start, s->offset, s->bytes, s->chars, s->encoding);
} // refer_to_whole

inline static void copy3(char_t * dst, const char_t * s1, int32_t b1, const char_t * s2, int32_t b2, const char_t * s3, int32_t b3)
{
    memcpy(dst, s1, b1);
    memcpy(dst + b1, s2, b2);
    memcpy(dst + b1 + b2, s3, b3);
    dst[b1 + b2 + b3] = 0;
} // copy3

nstr_p nstr_new(const char_t * src, int32_t bytes, bool copy)
{
    const char_t * start = blank_ent.data;
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t offset = 0;

    new = malloc(sizeof(nstr_t));
    if (! new) return NULL;

    if (src && bytes > 0) {
        if (copy) {
            ent = new_entity(bytes);
            if (! ent) {
                free(new);
                return NULL;
            } // if

            memcpy(ent->data, src, bytes);
            ent->data[bytes] = 0;

            start = ent->data;
            offset = 0;
        } else {
            start = src;
            offset = -1;
        } // if
    } // if

    return init_slice(new, true, start, offset, bytes, bytes, STR_ENC_ASCII);
} // nstr_new

nstr_p nstr_new_blank(str_encoding_t encoding)
{
    return new_slice(blank_ent.data, 0, 0, 0, encoding);
} // nstr_new_blank

nstr_p nstr_clone(nstr_p s)
{
    nstr_p new = nstr_new(s->start, s->bytes, true);
    if (new) nstr_to_encoding(new, s->encoding);
    return new;
} // nstr_clone

nstr_p nstr_duplicate(nstr_p s)
{
    return new_slice(s->start, s->offset, s->bytes, s->chars, s->encoding);
} // nstr_duplicate

void nstr_delete(nstr_p s)
{
    entity_p ent = NULL;

    if (! s) return; // NULL 指针

    ent = get_entity(s);
    if (s->need_free) free(s);

    del_ref(ent);
} // nstr_delete

void nstr_delete_array(nstr_array_p * as, int n)
{
    if (*as) {
        while (--n >= 0) nstr_delete((*as)[n]);
        free(*as);
        *as = NULL;
    } // if
} // nstr_delete_array

int32_t nstr_encoding(nstr_p s)
{
    return s->encoding;
} // nstr_encoding

int32_t nstr_bytes(nstr_p s)
{
    return s->bytes;
} // nstr_bytes

int32_t nstr_chars(nstr_p s)
{
    return s->chars;
} // nstr_chars

void nstr_byte_range(nstr_p s, const char_t ** start, const char_t ** end)
{
    *start = s->start;
    *end = s->start + s->bytes;
} // nstr_byte_range

int32_t nstr_next_char(nstr_p s, const char_t ** start, int32_t * index, nstr_p ch)
{
    int32_t bytes = 0;

    assert(s != NULL);
    assert(! nstr_is_blank(s));

    assert(ch != NULL);

    if (! *start) {
        *start = s->start;
        *index = 0;
        refer_to_other(ch, s->start, 0, 0, 1, s->encoding);
    } else {
        *start += ch->bytes;
        *index += 1;
    } // if

    bytes = vtable[s->encoding].measure(*start);
    if (bytes == 0) {
        *start = NULL;
        *index = s->chars;
        return STR_UNKNOWN_BYTE;
    } // if

    ch->offset += ch->bytes;
    ch->bytes = bytes;
    return bytes;
} // nstr_next_char

int32_t nstr_next_sub(nstr_p s, nstr_p sub, const char_t ** start, int32_t * index)
{
    const char_t * loc = NULL;  // 下个子串位置
    size_t size = 0;    // 搜索范围长度
    int32_t chars = 0;  // 距离上个子串位置的字符数
    int32_t bytes = 0;  // 距离上个子串位置的字节数

    assert(s != NULL);
    assert(! nstr_is_blank(s));

    assert(sub != NULL);
    assert(! nstr_is_blank(sub));

    assert(start != NULL);
    assert(index != NULL);

    if (! *start) {
        *start = s->start;
        size = s->bytes;
        *index = 0;
    } else {
        *start += sub->bytes;
        size = s->bytes - (*start - s->start);
        *index += sub->chars;
    } // if

    if (sub->bytes == 1) {
        // 子串只包含单个字节
        loc = memchr(*start, ((char_t *)sub->start)[0], size);
    } else {
        loc = memmem(*start, size, sub->start, sub->bytes);
    } // if
    if (! loc) {
        *start = NULL; // 停止查找
        *index = s->chars;
        return STR_NOT_FOUND; // 找不到子串
    } // if

    chars = s->chars; // 最大跳过字符数小于源串字符数
    bytes = vtable[s->encoding].count(*start, loc - *start, &chars);

    *index += chars;
    return bytes;
} // nstr_next_sub

int32_t nstr_to_encoding(nstr_p s, str_encoding_t encoding)
{
    int32_t r_bytes = 0;
    int32_t r_chars = 0;

    r_chars = s->bytes; // 字符数上限为字节数
    r_bytes = vtable[encoding].count(s->start, s->bytes, &r_chars);
    if (r_bytes >= 0) {
        // 编码正确
        s->chars = r_chars;
        s->encoding = encoding;
    } // if
    return r_bytes;
} // nstr_to_encoding

void nstr_narrow_down(nstr_p s, int32_t index, int32_t chars)
{
    const char_t * start = NULL;
    int32_t r_bytes = 0;
    int32_t r_chars = 0;

    // TODO: Support negtive index and chars.

    // CASE-1: 切片起点超出范围
    // CASE-2: 源串是空串
    // CASE-3: 切片长度是零
    if (s->chars <= index || chars == 0 || s->chars == 0) {
        s->bytes = 0;
        s->chars = 0;
        return;
    } // if

    // CASE-4: 源字符串不是空串
    start = s->start;
    if (0 < index) {
        // 跳过前导部分
        r_chars = index;
        r_bytes = vtable[s->encoding].count(start, s->bytes, &r_chars);
        start += r_bytes;
    } // if

    // 最大切片范围是剩余部分
    r_bytes = s->bytes - r_bytes;
    r_chars = s->chars - r_chars;
    if (chars < r_chars) {
        r_chars = chars;
        r_bytes = vtable[s->encoding].count(start, r_bytes, &r_chars);
    } // if

    s->start = start;
    s->bytes = r_bytes;
    s->chars = r_chars;
} // nstr_narrow_down

nstr_p nstr_slice(nstr_p s, int32_t index, int32_t chars, nstr_p r)
{
    if (r) {
        refer_to_other(r, s->start, s->offset, s->bytes, s->chars, s->encoding);
    } else {
        r = new_slice(s->start, s->offset, s->bytes, s->chars, s->encoding);
    } // if
    if (r) nstr_narrow_down(r, index, chars);
    return r;
} // nstr_slice

inline static int32_t augment_array(nstr_p ** as, int * cap, int delta)
{
    nstr_array_p an = realloc(*as, sizeof((*as)[0]) * (*cap + delta)); // 数组扩容
    if (! an) return 0;
    *cap += delta;
    *as = an;
    return STR_OUT_OF_MEMORY;
} // augment_array

int nstr_split(nstr_p s, nstr_p deli, int max, nstr_array_p * as)
{
    nstr_p new = NULL; // 新子串
    const char_t * start = NULL; // 本次搜索起始地址
    int32_t index = 0; // 本次搜索起始下标
    int32_t last = 0; // 上次搜索起始下标
    int32_t ret = 0; // 返回值 & 跳过字节数
    int rmd = 0; // 剩余切分次数，零表示停止，负数表示无限次
    int cnt = 0; // 子串数量，用于下标时始终指向下一个可用元素
    int cap = 0; // 数组容量
    int delta = 0; // 减量

    if (s->chars == 0) {
        // CASE-1: 源串是空串
        *as = malloc(sizeof(as[0]) * 2);
        if (! *as) return STR_OUT_OF_MEMORY;
        (*as)[0] = nstr_new_blank(s->encoding);
        (*as)[1] = NULL;
        return 1;
    } // if

    assert(deli && ! nstr_is_blank(deli));

    rmd = (max > 0) ? max : -1;
    delta = (rmd > 0) ? 1 : 0;

    // max 次分割将产生 max + 1 个子串，再加上 1 个 NULL 终止标志
    // 保留 2 个元素给最后的子串和终止标志
    cap = (rmd < 16 - 2) ? 16 : (rmd + 2);

    *as = malloc(sizeof((*as)[0]) * cap);
    if (! *as) return STR_OUT_OF_MEMORY;

    last = 0 - deli->chars;
    while (rmd != 0) {
        if (cnt >= cap - 2 && (ret = augment_array(as, &cap, 16)) < 0) goto NSTR_SPLIT_ERROR;

        ret = nstr_next_sub(s, deli, &start, &index);
        if (ret == STR_UNKNOWN_BYTE) goto NSTR_SPLIT_ERROR;
        if (ret == STR_NOT_FOUND) rmd = delta; // 退出查找

        new = new_slice(start, start - s->start, ret, index - (last + deli->chars), s->encoding);
        if (! new) {
            ret = STR_OUT_OF_MEMORY;
            goto NSTR_SPLIT_ERROR;
        } // if

        (*as)[cnt++] = new;
        last = index;
        rmd -= delta;
    } // while

    (*as)[cnt] = NULL; // 设置终止标志
    ret = cnt;
    return ret;

NSTR_SPLIT_ERROR:
    nstr_delete_array(as, cnt);
    return ret;
} // nstr_split

typedef char_t * (*copy_strings_t)(char_t * pos, nstr_p * as, int n, const char_t * dbuf, int32_t dbytes);

static char_t * copy_strings(char_t * pos, nstr_p * as, int n, const char_t * dbuf, int32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
    while (b-- > 0) {
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 3:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 2:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
        case 1:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
    } // while
        default: break;
    } // switch
    return pos;
} // copy_strings

static char_t * copy_strings_with_short_deli(char_t * pos, nstr_p * as, int n, const char_t * dbuf, int32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
    while (b-- > 0) {
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 3:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 2:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
        case 1:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            (*pos++) = dbuf[0];
    } // while
        default: break;
    } // switch
    return pos;
} // copy_strings_with_short_deli

static char_t * copy_strings_with_long_deli(char_t * pos, nstr_p * as, int n, const char_t * dbuf, int32_t dbytes)
{
    int i = 0;
    int b = n / 4;

    if (n <= 0) return pos;
    switch (n % 4) {
        case 0:
    while (b-- > 0) {
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 3:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 2:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
        case 1:
            memcpy(pos, (as[i])->start, (as[i])->bytes);
            pos += (as[i++])->bytes;
            memcpy(pos, dbuf, dbytes);
            pos += dbytes;
    } // while
        default: break;
    } // switch
    return pos;
} // copy_strings_with_long_deli

static entity_p join_strings(nstr_p deli, nstr_p * as, int n, va_list * ap, int32_t * chars)
{
    va_list cp;
    copy_strings_t copy = &copy_strings;
    entity_p ent = NULL;
    nstr_p * as2 = NULL;
    char_t * pos = NULL;
    const char_t * dbuf = NULL;
    int32_t bytes = 0;
    int32_t dbytes = 0;
    int i = 0;
    int n2 = 0;
    int cnt = 0;

    // 第一遍：计算总字节数
    cnt += n;
    for (i = 0; i < n; ++i) {
        bytes += as[i]->bytes;
        *chars += as[i]->chars;
    } // for

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        n2 = va_arg(cp, int);
        cnt += n2;
        for (i = 0; i < n2; ++i) {
            bytes += (as2[i])->bytes;
            *chars += (as2[i])->chars;
        } // for
    } // while
    va_end(cp);

    if (cnt == 0) return &blank_ent;

    if (deli && deli->bytes > 0) {
        dbuf = deli->start;
        dbytes = deli->bytes;

        bytes += dbytes * cnt; // 字节总数包含尾部间隔符，简化拷贝逻辑
        *chars += deli->chars * (cnt - 1); // 字符总数不包含尾部间隔符

        copy = (deli->bytes == 1) ? &copy_strings_with_short_deli : &copy_strings_with_long_deli;
    } // if

    ent = new_entity(bytes);
    if (! ent) return NULL;

    // 第二遍：拷贝字节数据
    pos = copy(ent->data, as, n, dbuf, dbytes);

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        pos = copy(pos, as2, va_arg(cp, int), dbuf, dbytes);
    } // while
    va_end(cp);

    ent->bytes -= dbytes; // 去掉多余的尾部间隔符
    ent->data[ent->bytes] = 0; // 设置终止 NUL 字符
    return ent;
} // join_strings

nstr_p nstr_repeat(nstr_p s, int n, nstr_p r)
{
    nstr_p as[16] = {
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
    };
    char_t * pos = NULL;
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t b = 0;

    if (s->bytes == 0 || n <= 1) return refer_to_whole(r, s); // CASE-1: s 是空串

    ent = new_entity(s->bytes * n);
    if (! ent) return NULL;

    pos = copy_strings(ent->data, as, n % (sizeof(as) / sizeof(as[0])), NULL, 0);

    b = n / (sizeof(as) / sizeof(as[0]));
    while (b-- > 0) pos = copy_strings(pos, as, (sizeof(as) / sizeof(as[0])), NULL, 0);

    new = new_slice(ent->data, 0, ent->bytes, s->bytes * n, s->encoding);
    if (! new) free(ent);
    return new;
} // repeat

nstr_p nstr_concat(nstr_p * as, int n, nstr_p r, ...)
{
    va_list ap;
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t chars = 0;

    va_start(ap, r);
    ent = join_strings(NULL, as, n, &ap, &chars);
    va_end(ap);
    if (! ent) return NULL;

    new = new_slice(ent->data, 0, ent->bytes, chars, as[0]->encoding);
    if (! new) free(ent);
    return new;
} // nstr_concat

nstr_p nstr_concat2(nstr_p s1, nstr_p s2, nstr_p r)
{
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes;
    if (bytes == 0) return refer_to_or_new_slice(r, blank_ent.data, 0, 0, 0, s1->encoding);

    ent = new_entity(bytes);
    if (! ent) return NULL;

    memcpy(ent->data, s1->start, s1->bytes);
    memcpy(ent->data + s1->bytes, s2->start, s2->bytes);
    ent->data[bytes] = 0;

    new = new_slice(ent->data, 0, ent->bytes, s1->chars + s2->chars, s1->encoding);
    if (! new) free(ent);
    return new;
} // nstr_concat2

nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3, nstr_p r)
{
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes + s3->bytes;
    if (bytes == 0) return refer_to_or_new_slice(r, blank_ent.data, 0, 0, 0, s1->encoding);

    ent = new_entity(bytes);
    if (! ent) return NULL;

    copy3(ent->data, s1->start, s1->bytes, s2->start, s2->bytes, s3->start, s3->bytes);

    new = new_slice(ent->data, 0, ent->bytes, s1->chars + s2->chars + s3->chars, s1->encoding);
    if (! new) free(ent);
    return new;
} // nstr_concat3

nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, nstr_p r, ...)
{
    va_list ap;
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t chars = 0;

    va_start(ap, r);
    ent = join_strings(deli, as, n, &ap, &chars);
    va_end(ap);
    if (! ent) return NULL;

    new = new_slice(ent->data, 0, ent->bytes, chars, as[0]->encoding);
    if (! new) free(ent);
    return new;
} // nstr_join

nstr_p nstr_join_by_char(char_t deli, nstr_p * as, int n, nstr_p r, ...)
{
    va_list ap;
    nstr_t d = {0};
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t chars = 0;

    init_slice(&d, false, &deli, -1, 1, 1, STR_ENC_ASCII);
    va_start(ap, r);
    ent = join_strings(&d, as, n, &ap, &chars);
    va_end(ap);
    del_ref(get_entity(&d));
    if (! ent) return NULL;

    new = new_slice(ent->data, 0, ent->bytes, chars, as[0]->encoding);
    if (! new) free(ent);
    return new;
} // nstr_join_by_char

nstr_p nstr_replace(nstr_p s, int32_t index, int32_t chars, nstr_p to, nstr_p r)
{
    entity_p ent = NULL;
    nstr_p new = NULL;
    int32_t p1_bytes = 0;
    int32_t p2_bytes = 0;
    int32_t p3_bytes = 0;
    int32_t bytes = 0;
    int32_t p1_chars = 0;
    int32_t p2_chars = 0;
    int32_t p3_chars = 0;

    // TODO: Support negtive index and chars.

    p1_chars = index; // 跳过部分
    p2_chars = s->chars - p1_chars; // 待替换部分
    if (chars < p2_chars) p2_chars = chars;
    p3_chars = s->chars - p1_chars - p2_chars;

    if (to->chars == 0) return refer_to_whole(r, s); // CASE: 替换部分零长度

    if (p1_chars > 0) p1_bytes = vtable[s->encoding].count(s->start, s->bytes, &p1_chars);
    if (p2_chars > 0) p2_bytes = vtable[s->encoding].count(s->start + p1_bytes, s->bytes - p1_bytes, &p2_chars);

    p3_bytes = s->bytes - p1_bytes - p2_bytes;
    bytes = p1_bytes + to->bytes + p3_bytes;

    ent = new_entity(bytes);
    if (! ent) return NULL;

    copy3(ent->data, s->start, p1_bytes, to->start, to->bytes, s->start + p1_bytes + p2_bytes, p3_bytes);

    new = refer_to_or_new_slice(r, ent->data, 0, bytes, p1_chars + to->chars + p3_chars, s->encoding);
    if (! new) free(ent);
    return new;
} // nstr_replace

nstr_p nstr_replace_with_char(nstr_p s, int32_t index, int32_t chars, char_t ch, nstr_p r)
{
    nstr_t to = {0};
    nstr_p new = NULL;

    init_slice(&to, false, &ch, -1, 1, 1, STR_ENC_ASCII);
    new = nstr_replace(s, index, chars, &to, r);
    del_ref(get_entity(&to));
    return new;
} // nstr_replace_with_char

nstr_p nstr_remove(nstr_p s, int32_t index, int32_t chars, nstr_p r)
{
    nstr_t b = {.start = blank_ent.data, .encoding = s->encoding };
    return nstr_replace(s, index, chars, &b, r);
} // nstr_remove

nstr_p nstr_cut_head(nstr_p s, int32_t chars, nstr_p r)
{
    nstr_t b = {.start = blank_ent.data, .encoding = s->encoding };
    // CASE-1: 删除长度大于字符串长度
    if (s->chars < chars) return refer_to_or_new_slice(r, blank_ent.data, 0, 0, 0, s->encoding);
    return nstr_replace(s, 0, chars, &b, r);
} // nstr_cut_head

nstr_p nstr_cut_tail(nstr_p s, int32_t chars, nstr_p r)
{
    nstr_t b = {.start = blank_ent.data, .encoding = s->encoding };
    // CASE-1: 删除长度大于字符串长度
    if (s->chars < chars) return refer_to_or_new_slice(r, blank_ent.data, 0, 0, 0, s->encoding);
    return nstr_replace(s, s->chars - chars, chars, &b, r);
} // nstr_cut_tail

nstr_p nstr_substitute(nstr_p s, bool all, nstr_p from, nstr_p to, nstr_p r)
{
    nstr_array_p as = NULL; // 子串数组
    const char_t * start = NULL; // 遍历变量
    int32_t p1_bytes = 0; // 跳过字节数
    int32_t index = 0; // 待替换串下标
    int cnt = 0; // 子串数

    if (all) {
        cnt = nstr_split(s, from, -1, &as);
        if (cnt < 0) return NULL;
        return nstr_join(to, as, cnt, r, NULL);
    } // if

    p1_bytes = nstr_next_sub(s, from, &start, &index);
    if (p1_bytes == STR_UNKNOWN_BYTE) return NULL;
    if (p1_bytes == STR_NOT_FOUND) return refer_to_whole(r, s);

    return nstr_replace(s, index, from->chars, to, r);
} // nstr_substitute
