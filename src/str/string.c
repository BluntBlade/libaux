#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "str/ascii.h"
#include "str/string.h"

#define container_of(type, member, addr) ((type *)((void *)(addr) - (void *)(&(((type *)0)->member))))

typedef int32_t (*measure_t)(void * pos);
typedef int32_t (*count_t)(void * start, int32_t size, int32_t * chars);
typedef bool (*verify_t)(void * start, int32_t size);

typedef struct VTABLE {
    measure_t   measure;        // 度量单个字符的字节数
    count_t     count;          // 计算字节范围包含的字符数
    verify_t    verify;         // 校验字节范围是否编码正确
} vtable_t, *vtable_p;

typedef struct NSTR {
    int32_t         bytes;          // 串内容占用字节数
    int32_t         chars;          // 编码后的字符个数

    uint32_t        unused:26;
    uint32_t        encoding:6;     // 编码方案，支持最多 64 种

    union {
        int32_t     slcs;           // （仅用于字符串）切片计数，减到 0 则销毁字符串并释放内存
        int32_t     offset;         // （仅用于切片）字符数据起始地址相对于源地址的偏移量，>= 0 表示引用 NSTR 字符串，< 0 表示引用 C 字符串
    };

    char_t *        start;          // 字符数据起始地址。A）字符串：指向自身的 data 成员；B）切片：指向被引用的字符串
    char_t          data[1];        // 字符存储区，包含结尾的 NUL 字符
} nstr_t;

// ---- 静态变量 ---- //

vtable_t vtable[NSTR_ENCODING_COUNT] = {
    {
        &ascii_measure,
        &ascii_count,
        &ascii_verify,
    },
    {
        &utf8_measure,
        &utf8_count,
        &utf8_verify,
    },
};

const char_t c_blank = "";
nstr_t blank = {.start = &c_blank, .encoding = STR_ENC_ASCII};

inline static is_slice(nstr_p s)
{
    return s->start != &s->data[0];
} // is_slice

inline static nstr_p get_entity(nstr_p s)
{
    return (is_slice(s) && s->offset >= 0) ? container_of(nstr_t, data, (s->start - s->offset)) : NULL;
} // get_entity

inline static nstr_p add_ref(nstr_p s)
{
    nstr_p ent = get_entity(s);
    if (ent) ent->refs += 1;
    return s;
} // add_ref

inline static void del_ref(nstr_p s)
{
    nstr_p ent = get_entity(s);
    if (ent && --ent->refs == 0) free(ent);
} // del_ref

static nstr_p new_entity(void * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    nstr_p new = malloc(sizeof(nstr_t) + bytes);
    if (new) {
        new->encoding = encoding;
        new->bytes = bytes;
        new->chars = chars;
        new->refs = 1;  // 引用自身
        new->start = &new->data[0];
        if (start) memcpy(new->data, start + offset, bytes);
    } // if
    return new;
} // new_entity

inline static nstr_p init_slice(nstr_p s, void * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    s->encoding = encoding;
    s->bytes = bytes;
    s->chars = chars;
    s->offset = offset;
    s->start = start;
    return add_ref(s);
} // init_slice

inline static void clean_slice(nstr_p s)
{
    del_ref(s);
    s->start = NULL;
    s->offset = 0;
} // clean_slice

static nstr_p new_slice(void * start, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    nstr_p new = malloc(sizeof(nstr_t));
    if (new) init_slice(new, start, offset, bytes, chars, encoding);
    return new;
} // new_slice

inline static nstr_p refer_to_new(nstr_p slc, nstr_p new)
{
    if (slc) {
        clean_slice(slc);
        init_slice(slc, new->start, 0, new->bytes, new->chars, new->encoding);
        return slc;
    } // if
    return new;
} // refer_to_new

nstr_p nstr_new(void * src, int32_t bytes, str_encoding_t encoding)
{
    nstr_p new = NULL;
    int32_t r_bytes = 0;
    int32_t r_chars = bytes;

    if (bytes == 0) return nstr_blank_string();

    r_bytes = vtable[encoding]->count(src, bytes, &r_chars);
    if (r_bytes < 0) return NULL;

    new = new_entity(src, 0, bytes, chars, encoding);
    return new;
} // nstr_new

nstr_p nstr_clone(nstr_p s)
{
    return nstr_new(s->start, s->bytes, s->encoding);
} // nstr_clone

nstr_p nstr_duplicate(nstr_p s)
{
    if (is_slice(s)) {
        return new_slice(s->start, s->offset, s->bytes, s->chars, s->encoding);
    } // if
    return nstr_new(s->start, s->bytes, s->encoding);
} // nstr_duplicate

nstr_p nstr_blank_string(void)
{
    return add_ref(&blank);
} // nstr_blank_string

nstr_p nstr_blank_slice(void)
{
    nstr_p new = malloc(sizeof(nstr_t));
    if (new) init_slice(new, NULL, 0, 0, 0, STR_ENC_ASCII);
    return new;
} // nstr_blank_slice

void nstr_delete(nstr_p s)
{
    nstr_p ent = s;

    if (! s) return; // 空指针
    if (is_slice(s)) {
        ent = get_entity(s);
        free(s); // 释放切片
    } // if
    if (ent) del_ref(ent);
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

bool nstr_is_string(nstr_p s)
{
    return (! is_slice(s));
} // nstr_is_string

bool nstr_is_slice(nstr_p s)
{
    return is_slice(s);
} // nstr_is_slice

bool nstr_is_blank(nstr_p s)
{
    return s->chars == 0;
} // nstr_is_blank

bool nstr_verify(nstr_p s)
{
    return vtable[s->encoding].verify(s->start, s->bytes);
} // nstr_verify

void nstr_byte_range(nstr_p s, const char_t ** start, const char_t ** end)
{
    *pos = s->start;
    *end = s->start + s->bytes;
} // nstr_byte_range

int32_t nstr_next_char(nstr_p s, const char_t ** start, int32_t * index, nstr_p ch)
{
    int32_t bytes = 0;

    assert(s != NULL);
    assert(! nstr_is_blank(s));

    assert(sub != NULL);
    assert(nstr_is_slice(ch));

    if (! *start) {
        *start = s->start;
        *index = 0;
        clean_slice(ch);
        init_slice(ch, s->start, 0, 0, 1, s->encoding);
    } else {
        *start += ch->bytes;
        *index += 1;
    } // if

    bytes = vtable[s->encoding].measure(*start, s->bytes - ch->offset - ch->bytes);
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
    int32_t skip = 0;   // 距离上个子串位置的字符数
    int32_t bytes = 0;  // 距离上个子串位置的字节数

    assert(s != NULL);
    assert(! nstr_is_blank(s));

    assert(sub != NULL);
    assert(nstr_is_slice(sub));
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

    skip = s->chars; // 最大跳过字符数小于源串字符数
    bytes = vtable[s->encoding].count(*start, loc - *start, &skip);
    if (bytes < 0) {
        *start = NULL; // 停止查找
        *index = s->chars;
        return STR_UNKNOWN_BYTE; // 源串包含异常字节
    } // if

    *index += skip;
    return bytes;
} // nstr_next_sub

nstr_p nstr_slice(nstr_p s, int32_t index, int32_t chars, nstr_p slc)
{
    const char_t * start = blank.data;
    int32_t r_bytes = 0;
    int32_t r_chars = 0;

    // TODO: Support negtive index and chars.

    // CASE-1: 切片起点超出范围
    // CASE-2: 源串是空串
    // CASE-3: 切片长度是零
    if (index >= s->chars || s->chars == 0 || chars == 0) goto NSTR_SLICE_END;

    // CASE-4: 源字符串不是空串
    start = s->start;
    if (0 < index) {
        // 跳过前导部分
        r_chars = index;
        r_bytes = vtable[s->encoding].count(start, s->bytes, &r_chars);
        if (r_bytes < 0) return NULL; // 编码不正确
        start += r_bytes;
    } // if

    // 最大切片范围是剩余部分
    r_bytes = s->bytes - r_bytes;
    r_chars = s->chars - r_chars;
    if (chars < r_chars) {
        r_chars = chars;
        r_bytes = vtable[s->encoding].count(start, r_bytes, &r_chars);
        if (r_bytes < 0) return NULL; // 编码不正确
    } // if

NSTR_SLICE_END:
    if (slc) {
        clean_slice(slc);
        return init_slice(slc, start, start - s->start, r_bytes, r_chars, s->encoding);
    } // if
    return new_slice(start, start - s->start, r_bytes, r_chars, s->encoding);
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
        if (! *as) return NULL;
        (*as)[0] = nstr_blank_string();
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

typedef char_t * (*copy_strings_t)(char_t * pos, nstr_p * as, int n, char_t * dbuf, int32_t dbytes);

static char_t * copy_strings(char_t * pos, nstr_p * as, int n, char_t * dbuf, int32_t dbytes)
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

static char_t * copy_strings_with_short_deli(char_t * pos, nstr_p * as, int n, char_t * dbuf, int32_t dbytes)
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

static char_t * copy_strings_with_long_deli(char_t * pos, nstr_p * as, int n, char_t * dbuf, int32_t dbytes)
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

static nstr_p join_strings(nstr_p deli, nstr_p as, int n, va_list * ap)
{
    va_list cp;
    copy_strings_t copy = &copy_strings;
    nstr_p new = NULL;
    nstr_p as2 = NULL;
    char_t * pos = NULL;
    char_t * dbuf = NULL;
    int32_t bytes = 0;
    int32_t chars = 0;
    int32_t dbytes = 0;
    int i = 0;
    int n2 = 0;
    int cnt = 0;

    // 第一遍：计算总字节数
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
            bytes += as2[i]->bytes;
            chars += as2[i]->chars;
        } // for
    } // while
    va_end(cp);

    if (cnt == 0) return nstr_blank_string();

    if (deli && deli->bytes > 0) {
        dbuf = deli->start;
        dbytes = deli->bytes;

        bytes += dbytes * cnt; // 字节总数包含尾部间隔符，简化拷贝逻辑
        chars += deli->chars * (cnt - 1); // 字符总数不包含尾部间隔符

        copy = (deli->bytes == 1) ? &copy_strings_with_short_deli : &copy_strings_with_long_deli;
    } // if

    new = new_entity(false, NULL, 0, bytes, chars, as[0]->encoding);
    if (! new) return NULL;

    // 第二遍：拷贝字节数据
    pos = copy(new->data, as, n, dbuf, dbytes);

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        pos = copy(pos, as2, va_args(cp, int), dbuf, dbytes);
    } // while
    va_end(cp);

    new->bytes -= dbytes; // 去掉多余的尾部间隔符
    new->data[new->bytes] = 0; // 设置终止 NUL 字符
    return new;
} // join_strings

nstr_p nstr_repeat(nstr_p s, int n, nstr_p slc)
{
    nstr_p as[16] = {
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
    };
    nstr_p new = NULL;
    int32_t b = 0;

    if (s->bytes == 0) return nstr_blank_string(); // CASE-1: s 是空串。
    if (n <= 1) return add_ref(s);

    new = new_entity(false, NULL, 0, (s->bytes * n), (s->chars * n), s->encoding);
    if (! new) return NULL;

    pos = copy_strings(new->data, as, n % (sizeof(as) / sizeof(as[0])), NULL, 0);

    b = n / (sizeof(as) / sizeof(as[0]));
    while (b-- > 0) pos = copy_strings(pos, as, (sizeof(as) / sizeof(as[0])), NULL, 0);

    return refer_to_new(slc, new);
} // repeat

nstr_p nstr_concat(nstr_p * as, int n, nstr_p slc, ...)
{
    va_list ap;
    nstr_p new = NULL;

    va_start(ap, slc);
    new = join_strings(NULL, as, n, &ap);
    va_end(ap);
    return refer_to_new(slc, new);
} // nstr_concat

nstr_p nstr_concat2(nstr_p s1, nstr_p s2, nstr_p slc)
{
    nstr_p new = NULL;
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes;
    if (bytes == 0) return nstr_blank_string();

    new = new_entity(false, NULL, 0, bytes, s1->chars + s2->chars, s1->encoding);
    if (! new) return NULL;

    memcpy(new->data, s1->start, s1->bytes);
    memcpy(new->data + s1->bytes, s2->start, s2->bytes);
    new->data[bytes] = 0;
    return refer_to_new(slc, new);
} // nstr_concat2

nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3, nstr_p slc)
{
    nstr_p new = NULL;
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes + s3->bytes;
    if (bytes == 0) return nstr_blank_string();

    new = new_entity(false, NULL, 0, bytes, s1->chars + s2->chars + s3->chars, s1->encoding);
    if (! new) return NULL;

    memcpy(new->data, s1->start, s1->bytes);
    memcpy(new->data + s1->bytes, s2->start, s2->bytes);
    memcpy(new->data + s1->bytes + s2->bytes, s3->start, s3->bytes);
    new->data[bytes] = 0;
    return refer_to_new(slc, new);
} // nstr_concat3

nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, nstr_p slc, ...)
{
    va_list ap;
    nstr_p new = NULL;

    va_start(ap, slc);
    new = join_strings(deli, as, n, &ap);
    va_end(ap);
    return refer_to_new(slc, new);
} // nstr_join

nstr_p nstr_join_by_char(char_t deli, nstr_p * as, int n, nstr_p slc, ...)
{
    va_list ap;
    nstr_t d = {0};
    nstr_p new = NULL;

    d->encoding = STR_ENC_ASCII;
    d->bytes = 1;
    d->chars = 1;
    d->data[0] = deli;

    va_start(ap, slc);
    new = join_strings(d, as, n, &ap);
    va_end(ap);
    return refer_to_new(slc, new);
} // nstr_join_by_char

nstr_p nstr_replace(nstr_p s, int32_t index, int32_t chars, nstr_p to, nstr_p slc)
{
    nstr_p ent = NULL; // 最终字符串对象
    const char_t * start = NULL;
    int32_t p1_bytes = 0;
    int32_t p2_bytes = 0;
    int32_t p1_chars = 0;
    int32_t p2_chars = 0;

    // TODO: Support negtive index and chars.

    if (s->chars == 0) return nstr_duplicate(to); // CASE-1: 源串为空 
    if (to->chars == 0) return nstr_slice(s, 0, s->chars, NULL); // CASE-2: 子串为空
    if (index >= s->chars) return nstr_concat2(s, to); // CASE-3: 替换位置在串尾

    if (index == 0) {
        if (chars == 0) return nstr_concat2(to, s); // CASE-4: 替换位置在串头且替换长度为零
        if (chars == s->chars) return nstr_slice(s, 0, s->chars, NULL); // CASE-5: 替换整个源串
    } // if

    // CASE-6: 待替换部分在串头或串中且长度不为零
    p1_chars = index;
    p1_bytes = vtable[s->encoding].count(s->start, s->bytes, &p1_chars);
    if (p1_bytes < 0) return NULL; // 编码不正确

    start = s->start + p1_bytes;
    p2_chars = s->chars - p1_chars;
    p2_bytes = vtable[s->encoding].count(start, s->bytes - p1_bytes, &p2_chars);
    if (p2_bytes < 0) return NULL; // 编码不正确

    new = new_entity(false, NULL, 0, s->bytes - p2_bytes + to->bytes, s->chars - p2_chars + to->chars, s->encoding);
    if (! new) return NULL;

    memcpy(new->data, s->start, p1_bytes);
    memcpy(new->data + p1_bytes, to->start, to->bytes);
    memcpy(new->data + p1_bytes + to->bytes, start + p2_bytes, s->bytes - p1_bytes - p2_bytes);
    new->data[s->bytes - p2_bytes + to->bytes] = 0;
    return refer_to_new(slc, new);
} // nstr_replace

nstr_p nstr_replace_with_char(nstr_p s, int32_t index, int32_t chars, char_t ch, nstr_p slc)
{
    nstr_t to = {0};

    to->encoding = STR_ENC_ASCII;
    to->bytes = 1;
    to->chars = 1;
    to->data[0] = ch;
    return nstr_replace(s, index, chars, &to, slc);
} // nstr_replace_with_char

nstr_p nstr_remove(nstr_p s, int32_t index, int32_t chars, nstr_p slc)
{
    return nstr_replace(s, index, chars, &blank, slc);
} // nstr_remove

nstr_p nstr_cut_head(nstr_p s, int32_t chars, nstr_p slc)
{
    if (s->chars < chars) return refer_to_new(slc, &blank); // CASE-1: 删除长度大于字符串长度
    return nstr_replace(s, 0, chars, &blank, slc);
} // nstr_cut_head

nstr_p nstr_cut_tail(nstr_p s, int32_t chars, nstr_p slc)
{
    if (s->chars < chars) return refer_to_new(slc, &blank); // CASE-1: 删除长度大于字符串长度
    return nstr_replace(s, s->chars - chars, chars, &blank, slc);
} // nstr_cut_tail

nstr_p nstr_substitute(nstr_p s, bool all, nstr_p from, nstr_p to, nstr_p slc)
{
    nstr_array_p as = NULL; // 子串数组
    nstr_p new = NULL; // 新串
    const char_t * start = NULL; // 遍历变量
    int32_t skip = 0; // 跳过字节数
    int32_t index = 0; // 待替换串下标
    int cnt = 0; // 子串数

    if (all) {
        cnt = nstr_split(s, false, from, -1, &as);
        if (cnt < 0) return NULL;
        return nstr_join(to, as, cnt, NULL);
    } // if

    skip = nstr_next_sub(s, from, &start, &index);
    if (skip == STR_UNKNOWN_BYTE) return NULL;
    if (skip == STR_NOT_FOUND) return nstr_slice(s, true, 0, s->chars);

    new = new_entity(NULL, 0, s->bytes - from->bytes + to->bytes, s->chars - from->chars + to->chars, s->encoding);
    if (new) {
        memcpy(new->start, s->start, skip);
        memcpy(new->start + skip, to->start, to->bytes);
        memcpy(new->start + skip + to->bytes, s->start + skip + from->bytes, s->bytes - skip - from->bytes);
        new->start[s->bytes - from->bytes + to->bytes] = 0;
    } // if
    return refer_to_new(slc, new);
} // nstr_substitute
