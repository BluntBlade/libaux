#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "str/ascii.h"
#include "str/string.h"

#define container_of(type, member, addr) ((type *)((void *)(addr) - (void *)(&(((type *)0)->member))))
#define string_entity(s) ((s)->ref_cstr ? NULL : container_of(nstr_t, str.data, (s)->slc.origin))

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

    uint32_t        is_slice:1;     // 类型标志位：0 表示字符串，1 表示切片
    uint32_t        ref_cstr:1;     // 分片引用C字符串
    uint32_t        iterating:1;    // 是否正在遍历查找
    uint32_t        unused:23;
    uint32_t        encoding:6;     // 编码方案，支持最多 64 种

    union {
        struct {
            int32_t     refs;       // 引用计数，生成字符串时置 1 ，表示对自身的引用。减到 0 时释放内存
            char_t      data[1];    // 串内容存储区，包含结尾的 NUL 字符
        } str;
        struct {
            int32_t     offset;     // 首字节在源串内容存储区的偏移位置（单位：字节）
            char_t *    origin;     // 源串数据起始地址，加上 offset 得到切片数据起始地址
        } slc;
    };
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

nstr_t blank = {0};

inline static void * get_origin(nstr_p s)
{
    return (s->is_slice) ? s->slc.origin : s->str.data;
} // get_origin

inline static void * get_start(nstr_p s)
{
    return (s->is_slice) ? s->slc.origin + s->slc.offset : s->str.data;
} // get_start

inline static void * get_end(nstr_p s)
{
    return get_start(s) + s->bytes;
} // get_end

inline static int32_t get_offset(nstr_p s)
{
    return (s->is_slice) ? s->slc.offset : 0;
} // get_offset

inline static nstr_p get_entity(nstr_p s)
{
    return (s->is_slice) ? string_entity(s) : s;
} // get_entity

inline static nstr_p add_ref(nstr_p s)
{
    nstr_p ent = get_entity(s);
    if (ent) ent->str.refs += 1;
    return s;
} // add_ref

inline static void del_ref(nstr_p s)
{
    nstr_p ent = get_entity(s);
    if (ent && --ent->str.refs == 0) free(ent);
} // del_ref

static nstr_p new_entity(bool ref_cstr, void * origin, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    nstr_p new = malloc(sizeof(nstr_t) + bytes);
    if (new) {
        new->is_slice = 0;
        new->ref_cstr = 0;
        new->encoding = encoding;
        new->bytes = bytes;
        new->chars = chars;
        new->str.refs = 1;  // 引用自身
        if (origin) memcpy(new->str.data, origin + offset, bytes);
    } // if
    return new;
} // new_entity

inline static void init_slice(nstr_p s, bool ref_cstr, void * origin, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    s->is_slice = 1;
    s->ref_cstr = ref_cstr;
    s->iterating = 0;
    s->encoding = encoding;
    s->bytes = bytes;
    s->chars = chars;
    s->slc.offset = offset;
    s->slc.origin = origin;
    add_ref(s);
} // init_slice

inline static void clean_slice(nstr_p s)
{
    del_ref(s);
    s->ref_cstr = 1;
    s->slc.origin = NULL;
} // clean_slice

static nstr_p new_slice(bool ref_cstr, void * origin, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    nstr_p new = malloc(sizeof(nstr_t));
    if (new) init_slice(new, ref_cstr, origin, offset, bytes, chars, encoding);
    return new;
} // new_slice

nstr_p nstr_new(void * src, int32_t bytes, str_encoding_t encoding)
{
    nstr_p new = NULL;
    int32_t r_bytes = 0;
    int32_t r_chars = bytes;

    if (bytes == 0) return nstr_blank_string();

    r_bytes = vtable[encoding]->count(src, bytes, &r_chars);
    if (r_bytes < 0) return NULL;

    new = new_entity(false, src, 0, bytes, chars, encoding);
    return new;
} // nstr_new

nstr_p nstr_clone(nstr_p s)
{
    return nstr_new(get_start(s), s->bytes, s->encoding);
} // nstr_clone

nstr_p nstr_duplicate(nstr_p s)
{
    if (s->is_slice) {
        return new_slice(s->ref_cstr, get_origin(s), get_offset(s), s->bytes, s->chars, s->encoding);
    } // if
    return nstr_new(get_start(s), s->bytes, s->encoding);
} // nstr_duplicate

void nstr_delete(nstr_p * ps)
{
    nstr_p s = *ps;

    if (! s) return; // 空指针
    if (s->is_slice) {
        s = string_entity(*ps);
        free(*ps); // 释放切片
    } // if

    *ps = NULL; // 防止野指针
    del_ref(s);
} // nstr_delete

void nstr_delete_array(nstr_array_p * as, int n)
{
    if (*as) {
        while (--n >= 0) nstr_delete((*as)[n]);
        free(*as);
        *as = NULL;
    } // if
} // nstr_delete_array

nstr_p nstr_add_ref(nstr_p s)
{
    return add_ref(s);
} // nstr_add_ref

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

void * nstr_to_cstr(nstr_p * ps)
{
    nstr_p new = NULL;
    nstr_p s = *ps;

    if (s->is_slice) {
        new = nstr_clone(*ps);
        if (! new) return NULL;
        *ps = new;
    } // if
    return (*ps)->str.data;
} // nstr_to_cstr

int32_t nstr_refs(nstr_p s)
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

bool nstr_is_slicing(nstr_p s, nstr_p s2)
{
    return get_origin(s) == get_origin(s2);
} // nstr_is_slice

bool nstr_is_blank(nstr_p s)
{
    return s->chars == 0;
} // nstr_is_blank

bool nstr_verify(nstr_p s)
{
    return vtable[s->encoding].verify(get_start(s), s->bytes);
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

int32_t nstr_next_char(nstr_p s, nstr_p ch)
{
    int32_t bytes = 0;

    assert(s != NULL);
    assert(! nstr_is_blank(s));

    assert(sub != NULL);
    assert(nstr_is_slice(ch));

    if (! ch->iterating) {
        start = get_start(s);
        size = s->bytes;
    } else {
        start = get_start(ch) + ch->bytes;
        size = s->bytes - ch->slc.offset - ch->bytes;
    } // if

    bytes = vtable[s->encoding].measure(start, size);
    if (bytes == 0) {
        ch->iterating = 0;
        return STR_UNKNOWN_BYTE;
    } // if

    if (! ch->iterating) {
        if (! nstr_is_slicing(ch, s)) clean_slice(ch);
        init_slice(ch, s->ref_cstr, get_origin(s), get_offset(s), bytes, 1, s->encoding);
        ch->iterating = 1;
        return 0;
    } // if
    ch->slc.offset += ch->bytes;
    ch->bytes = bytes;
    return 1;
} // nstr_next_char

int32_t nstr_next_sub(nstr_p s, nstr_p sub)
{
    void * loc = NULL;
    void * start = NULL;
    size_t size = 0;
    int32_t skip = 0;
    int32_t bytes = sub->bytes;
    int32_t chars = sub->chars;

    assert(s != NULL);
    assert(! nstr_is_blank(s));

    assert(sub != NULL);
    assert(nstr_is_slice(sub));
    assert(! nstr_is_blank(sub));

    if (! sub->iterating) {
        start = get_start(s);
        size = s->bytes;
    } else {
        start = get_start(sub);
        size = s->bytes - sub->slc.offset - sub->bytes;
    } // if

    if (sub->bytes == 1) {
        // 子串只包含单个字节
        loc = memchr(start, ((char_t *)get_start(sub))[0], size);
    } else {
        loc = memmem(start, size, get_start(sub), sub->bytes);
    } // if
    if (! loc) {
        sub->iterating = 0; // 停止查找
        return STR_NOT_FOUND; // 找不到子串
    } // if

    skip = s->chars; // 最大跳过字符数小于源串字符数
    bytes = vtable[s->encoding].count(start, loc - start, &skip);
    if (bytes < 0) {
        sub->iterating = 0; // 停止查找
        return STR_UNKNOWN_BYTE; // 源串包含异常字节
    } // if

    if (! sub->iterating) {
        if (! nstr_is_slicing(sub, s)) {
            bytes = sub->bytes;
            chars = sub->chars;

            clean_slice(sub);
            init_slice(sub, s->ref_cstr, get_origin(s), get_offset(s) + (loc - start), bytes, chars, s->encoding);
        } // if

        sub->iterating = 1; // 开始查找
        return skip;
    } // if
    sub->slc.offset += sub->bytes + (loc - start);
    return skip + sub->chars;
} // nstr_next_sub

nstr_p nstr_blank_string(void)
{
    return add_ref(&blank);
} // nstr_blank_string

// 获取空分片
nstr_p nstr_blank_slice(void)
{
    nstr_p new = malloc(sizeof(nstr_t));
    if (new) init_slice(new, true, NULL, 0, 0, 0, STR_ENC_ASCII);
    return new;
} // nstr_blank_slice

nstr_p nstr_slice(nstr_p s, bool can_new, int32_t index, int32_t chars);
{
    void * start = NULL;
    int32_t r_bytes = 0;
    int32_t r_chars = 0;

    // CASE-1: 切片起点超出范围
    // CASE-2: 源串是空串
    // CASE-3: 切片长度是零
    // NOTE: 不生成切片，直接返回空串
    if (index >= s->chars || s->chars == 0 || chars == 0) return nstr_blank_string();

    // CASE-4: 源字符串不是空串
    start = get_start(s);
    if (index > 0) {
        r_chars = index;
        r_bytes = vtable[s->encoding].count(start, s->bytes, &r_chars);
        if (r_bytes == 0) return nstr_blank_string(); // index 超出串尾
        if (r_bytes < 0) return NULL; // 编码不正确
        start += r_bytes;
    } // if

    r_chars = s->chars - index; // 最大切片范围是整个源串
    r_bytes = vtable[s->encoding].count(start, s->bytes - r_bytes, &r_chars);
    if (r_bytes == 0) return nstr_blank_string(); // 超出串尾
    if (r_bytes < 0) return NULL; // 编码不正确

    if (can_new) return nstr_new(start, r_bytes, s->encoding);
    return new_slice(s->ref_cstr, get_origin(s), get_offset(s) + (start - get_start(s)), r_bytes, r_chars, s->encoding);
} // nstr_slice

inline static int32_t augment_array(nstr_p ** as, int * cap, int delta)
{
    nstr_array_p an = realloc(*as, sizeof((*as)[0]) * (*cap + delta)); // 数组扩容
    if (! an) return 0;
    *cap += delta;
    *as = an;
    return STR_OUT_OF_MEMORY;
} // augment_array

int nstr_split(nstr_p s, bool can_new, nstr_p deli, int max, nstr_array_p * as)
{
    nstr_t prev = {0}; // 分隔符切片
    nstr_t curr = {0}; // 分隔符切片

    nstr_p new = NULL; // 新子串
    int32_t index = 0; // 下次搜索起始下标
    int32_t bytes = 0; // 子串字节数
    int32_t chars = 0; // 子串字符数
    int32_t ret = 0; // 返回值
    int rmd = 0; // 剩余切分次数，零表示停止，负数表示无限次
    int cnt = 0; // 子串数量，用于下标时始终指向下一个可用元素
    int cap = 0; // 数组容量
    int delta = 0; // 减量

    nstr_p (*create)(bool, void *, int32_t, int32_t, int32_t, str_encoding_t) = (can_new) ? &new_entity : &new_slice;

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
    if (! *as) goto NSTR_SPLIT_END;

    init_slice(&prev, s->ref_cstr, get_origin(s), 0, 0, 0, s->encoding);
    init_slice(&curr, deli->ref_cstr, get_origin(deli), get_offset(deli), deli->bytes, deli->chars, s->encoding);
    while (rmd != 0) {
        if (cnt >= cap - 2 && (ret = augment_array(as, &cap, 16)) < 0) goto NSTR_SPLIT_ERROR;

        chars = ret = nstr_next_sub(s, &curr);
        if (ret == STR_UNKNOWN_BYTE) goto NSTR_SPLIT_ERROR;
        if (ret == STR_NOT_FOUND) {
            bytes = get_end(s) - get_end(prev);
            ret = s->chars - index - prev->chars;
            rmd = delta; // 退出查找
        } else {
            bytes = get_start(curr) - get_end(prev);
            index += ret + curr->chars;
        } // if

        new = create(s->ref_cstr, get_origin(s), get_offset(prev) + prev->bytes, bytes, chars, s->encoding);
        if (! new) {
            ret = STR_OUT_OF_MEMORY;
            goto NSTR_SPLIT_ERROR;
        } // if

        (*as)[cnt++] = new;
        rmd -= delta;
        prev = curr;
    } // while

    (*as)[cnt] = NULL; // 设置终止标志
    ret = cnt;

NSTR_SPLIT_END:
    del_ref(&curr);
    del_ref(&prev);
    return ret;

NSTR_SPLIT_ERROR:
    nstr_delete_array(as, cnt);
    goto NSTR_SPLIT_END;
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

static char_t * copy_strings_with_one_deli(char_t * pos, nstr_p * as, int n, char_t * dbuf, int32_t dbytes)
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

static char_t * copy_strings_with_long_deli(char_t * pos, nstr_p * as, int n, char_t * dbuf, int32_t dbytes)
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
    int32_t bytes = 0;
    int32_t chars = 0;
    int32_t dbytes = 0;
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

    if (cnt == 0) return nstr_blank_string();

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

    new = new_entity(false, NULL, 0, bytes, chars, as[0]->encoding);
    if (! new) return NULL;

    pos = copy(new->str.data, as, n, dbuf, dbytes);

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        pos = copy(pos, as2, va_args(cp, int), dbuf, dbytes);
    } // while
    va_end(cp);

    bytes -= dbytes; // 去掉多余的尾部间隔符。
    new->str.data[bytes] = 0; // 设置终止 NUL 字符。
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
    int32_t b = 0;

    if (s->bytes == 0) return nstr_blank_string(); // CASE-1: s 是空串。
    if (n <= 1) return add_ref(s);

    new = new_entity(false, NULL, 0, (s->bytes * n), (s->chars * n), s->encoding);
    if (! new) return NULL;

    pos = copy_strings(new->str.data, as, n % (sizeof(as) / sizeof(as[0])), NULL, 0);

    b = n / (sizeof(as) / sizeof(as[0]));
    while (b-- > 0) pos = copy_strings(pos, as, (sizeof(as) / sizeof(as[0])), NULL, 0);
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
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes;
    if (bytes == 0) return nstr_blank_string();

    new = new_entity(false, NULL, 0, bytes, s1->chars + s2->chars, s1->encoding);
    if (! new) return NULL;

    memcpy(new->str.data, get_start(s1), s1->bytes);
    memcpy(new->str.data + s1->bytes, get_start(s2), s2->bytes);
    return new;
} // nstr_concat2

nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3)
{
    nstr_p new = NULL;
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes + s3->bytes;
    if (bytes == 0) return nstr_blank_string();

    new = new_entity(false, NULL, 0, bytes, s1->chars + s2->chars + s3->chars, s1->encoding);
    if (! new) return NULL;

    memcpy(new->str.data, get_start(s1), s1->bytes);
    memcpy(new->str.data + s1->bytes, get_start(s2), s2->bytes);
    memcpy(new->str.data + s1->bytes + s2->bytes, get_start(s3), s3->bytes);
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

    d->encoding = STR_ENC_ASCII;
    d->bytes = 1;
    d->chars = 1;
    d->str.data[0] = deli;

    va_start(ap, n);
    new = join_strings(d, as, n, &ap);
    va_end(ap);
    return new;
} // nstr_join_with_char

nstr_p nstr_replace(nstr_p s, bool can_new, int32_t index, int32_t chars, nstr_p sub)
{
    nstr_t s1 = {0};
    nstr_t s3 = {0};
    nstr_p ent = NULL; // 最终字符串对象
    void * origin = NULL;
    void * start = NULL;
    int32_t offset = 0;
    int32_t r_bytes = 0;
    int32_t r_chars = 0;

    if (s->chars == 0) return nstr_slice_from(sub, can_new, get_start(sub), sub->bytes); // CASE-1: s 是空串。
    if (sub->chars == 0) return nstr_slice_from(s, can_new, get_start(s), s->bytes); // CASE-2: sub 是空串。

    if (index >= s->chars) return nstr_concat2(s, sub); // CASE-3: 替换位置在串尾。
    if (index == 0 && chars == 0) return nstr_concat2(sub, s); // CASE-4: 替换位置在串头且替换长度为零。

    // CASE-5: 待替换部分在源串中间且长度不为零。
    if (s->is_slice) {
        ent = string_entity(s);
        offset = s->slc.offset;
    } else {
        ent = s;
        offset = 0;
    } // if

    r_chars = index;
    r_bytes = vtable[s->encoding].count(get_start(s), s->bytes, &r_chars);
    if (r_bytes == 0) return nstr_duplicate(s); // 超出串尾
    if (r_bytes < 0) return NULL; // 编码不正确

    start = origin + r_bytes;
    r_chars = chars;
    r_bytes = vtable[s->encoding].count(start, s->bytes - r_bytes, &r_chars);
    if (r_bytes == 0) return nstr_duplicate(s); // 超出串尾
    if (r_bytes < 0) return NULL; // 编码不正确

    s1_bytes = start - origin;
    init_slice(&s1, s1->ref_cstr, ent->str.data, offset, s1_bytes, index, s1->encoding);
    init_slice(&s3, s3->ref_cstr, ent->str.data, offset + s1_bytes + r_bytes, s->bytes - s1_bytes - r_bytes, s->chars - index - r_chars, s3->encoding);
    new = nstr_concat3(s1, sub, s2);
    clean_slice(&s3);
    clean_slice(&s1);
    return new;
} // nstr_replace

nstr_p nstr_replace_char(nstr_p s, bool can_new, int32_t index, int32_t chars, char_t ch)
{
    nstr_t sub = {0};

    sub->encoding = STR_ENC_ASCII;
    sub->bytes = 1;
    sub->chars = 1;
    sub->str.data[0] = ch;
    return nstr_replace(s, can_new, index, chars, &sub);
} // nstr_replace_char

nstr_p nstr_remove(nstr_p s, bool can_new, int32_t index, int32_t chars)
{
    return nstr_replace(s, can_new, index, chars, &blank);
} // nstr_remove

nstr_p nstr_cut_head(nstr_p s, bool can_new, int32_t chars)
{
    if (s->chars < chars) return nstr_blank_string(); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, can_new, 0, chars, &blank);
} // nstr_cut_head

nstr_p nstr_cut_tail(nstr_p s, bool can_new, int32_t chars)
{
    if (s->chars < chars) return nstr_blank_string(); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, can_new, s->chars - chars, chars, &blank);
} // nstr_cut_tail
