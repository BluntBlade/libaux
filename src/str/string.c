#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "str/ascii.h"
#include "str/string.h"

#define container_of(type, member, addr) ((type *)((void *)(addr) - (void *)(&(((type *)0)->member))))
#define string_entity(s) container_of(nstr_t, str.data, s->slc.origin)

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

    uint32_t        need_free:1;    // 是否释放内存：0 表示不需要，1 表示需要
    uint32_t        is_slice:1;     // 类型标志位：0 表示字符串，1 表示切片
    uint32_t        cstr_src:1;     // 分片引用C字符串
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

nstr_t blank = { .str = { .vtbl = &vtable[0] } };

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

// 字符串对象占用字节数
inline static size_t entity_size(size_t bytes)
{
    return sizeof(nstr_t) + bytes;
} // entity_size

// 初始化字符串对象
inline static void init_entity(nstr_p s, bool can_free, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    s->need_free = can_free ? 1 : 0;
    s->is_slice = 0;
    s->cstr_src = 0;
    s->encoding = encoding;
    s->bytes = bytes;
    s->chars = chars;
    s->str.refs = 1;  // 引用自身
} // init_entity

// 功能：返回切片对象应占字节数
inline static size_t slice_size(void)
{
    return sizeof(nstr_t);
} // slice_size

// 功能：初始化切片对象
inline static void init_slice(nstr_p s, bool can_free, void * origin, int32_t offset, int32_t bytes, int32_t chars, str_encoding_t encoding)
{
    s->need_free = can_free ? 1 : 0;
    s->is_slice = 1;
    s->cstr_src = 0;
    s->encoding = encoding;
    s->bytes = bytes;
    s->chars = chars;
    s->slc.offset = offset;
    s->slc.origin = origin;
    nstr_add_ref(string_entity(s));
} // init_slice

inline static void clean_slice(nstr_p s)
{
    nstr_p ent = get_entity(s);
    s->refs -= 1;
    if (ent->str.refs == 0 && ent->need_free) free(ent); // 释放非空字符串
    s->slc.origin = NULL;
} // clean_slice

nstr_p nstr_new(void * src, int32_t bytes, str_encoding_t encoding)
{
    nstr_p new = NULL;
    int32_t r_bytes = 0;
    int32_t r_chars = bytes;

    if (bytes == 0) return nstr_blank();

    r_bytes = vtbl->count(src, bytes, &r_chars);
    if (r_bytes < 0) return NULL;

    if ((new = malloc(entity_size(bytes)))) {
        memcpy(new->str.data, src, bytes);
        init_entity(new, STR_NEED_FREE, bytes, r_chars, encoding);
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

    if (! ent) return; // 空指针
    if (ent->is_slice) {
        ent = (ent->slc.is_cstr) ? NULL : string_entity(*ps);
        if ((*ps)->need_free) {
            free(*ps); // 释放切片
            *ps = NULL; // 防止野指针
        } else {
            // 调用者分配的内存无须释放，无须防止野指针
        } // if
    } else {
        *ps = NULL; // 防止野指针
    } // if

    if (ent) {
        ent->str.refs -= 1;
        if (ent->str.refs == 0 && ent->need_free) free(ent); // 释放非空字符串
    } // if
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
    get_entity(s)->str.refs += 1;
    return s;
} // nstr_add_ref

int32_t nstr_encoding(nstr_p s)
{
    return (s->vtbl - &vtable);
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
    return s->vtbl.verify(get_start(s), s->bytes);
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

int32_t nstr_first_char(nstr_p ch, nstr_p s)
{
    int32_t r_bytes = 0;
    int32_t r_chars = 1;

    assert(nstr_is_slice(ch));

    if (s->chars == 0) return STR_NOT_FOUND; // 源串为空

    r_bytes = vtable[s->encoding].count(get_start(s), s->bytes, &r_chars);
    if (r_bytes < 0) return STR_UNKNOWN_BYTE;

    if (! nstr_is_slicing(ch, s)) clean_slice(ch);
    init_slice(ch, ch->need_free, get_entity(s)->str.data, get_offset(s), r_bytes, r_chars);
    return 1;
} // nstr_first_char

int32_t nstr_next_char(nstr_p ch, nstr_p s)
{
    int32_t r_bytes = 0;
    int32_t r_chars = 1;

    assert(nstr_is_slice(ch));
    assert(nstr_is_slicing(ch, s));

    ch->slc.offset += ch->bytes;

    r_bytes = vtable[s->encoding].count(ch->slc.origin + ch->slc.offset, s->bytes - ch->offset, &r_chars);
    if (r_bytes > 0) {
        ch->bytes = r_bytes;
        return 1;
    } // if
    return (r_bytes == 0) ? STR_NOT_FOUND : STR_UNKNOWN_BYTE;
} // nstr_next_char

int32_t nstr_first_sub(nstr_p s, nstr_p sub, nstr_p * slice)
{
    void * loc = NULL;
    int32_t index = 0;
    int32_t bytes = 0;
    int32_t chars = 0;

    if (s->chars == 0) return STR_NOT_FOUND; // 源串为空，找不到子串

    if (sub->bytes == 0) {
        // 空子串
        loc = get_start(s);
    } else {
        if (sub->bytes == 1) {
            // 子串只包含单个字节
            loc = memchr(get_start(s), ((char_t *)get_start(sub))[0], sub->bytes);
        } else {
            loc = memmem(get_start(s), s->bytes, get_start(sub), sub->bytes);
        } // if
        if (! loc) return STR_NOT_FOUND; // 找不到子串

        index = s->vtbl.count(get_start(s), loc - get_start(s));
        if (index < 0) return STR_UNKNOWN_BYTE; // 源串包含异常字节
        bytes = sub->bytes;
        chars = sub->chars;
    } // if

    *slice = malloc(slice_size());
    if (! *slice) return STR_OUT_OF_MEMORY; // 内存不足

    init_slice(*slice, STR_NEED_FREE, get_start(s), loc - get_start(s), bytes, chars);
    return index;
} // nstr_first_sub

int32_t nstr_next_sub(nstr_p s, nstr_p sub, nstr_p * slice)
{
    void * start = NULL;
    void * end = NULL;
    void * loc = NULL;
    int32_t bytes = 0;
    int32_t chars = 0;
    
    start = get_end(*slice);
    end = get_end(s);
    if (start == end) goto NSTR_NEXT_SUB_NOT_FOUND;

    if (sub->bytes == 0) {
        bytes = s->vtbl.measure(start);
        if (bytes == 0) goto NSTR_NEXT_SUB_UNKNOWN_BYTE;

        (*slice)->slc.offset += (*slice)->bytes;
        (*slice)->bytes = bytes;
        return (*slice)->slc.index += 1;
    } // if

    if (sub->bytes == 1) {
        // 子串只包含单个字节
        loc = memchr(start, ((char_t *)get_start(sub))[0], sub->bytes);
    } else {
        loc = memmem(start, end - start, get_start(sub), sub->bytes);
    } // if
    if (! loc) goto NSTR_NEXT_SUB_NOT_FOUND;

    chars = s->vtbl.count(start, loc - start);
    if (chars < 0) goto NSTR_NEXT_SUB_UNKNOWN_BYTE;

    (*slice)->slc.offset += (*slice)->bytes + (loc - start);
    return (*slice)->slc.index += (*slice)->chars + chars;

NSTR_NEXT_SUB_NOT_FOUND:
    nstr_delete(slice);
    return STR_NOT_FOUND; // 找不到子串

NSTR_NEXT_SUB_UNKNOWN_BYTE:
    nstr_delete(slice);
    return STR_UNKNOWN_BYTE; // 源串包含异常字节
} // nstr_next_sub

nstr_p nstr_blank(void)
{
    return nstr_add_ref(&blank);
 // nstr_blank

inline static nstr_p new_slice(nstr_p s, int32_t offset, int32_t bytes, int32_t chars)
{
    nstr_p new = NULL;

    new = malloc(slice_size());
    if (new) init_slice(new, STR_NEED_FREE, get_start(s), offset, bytes, chars);
    return new;
} // new_slice

nstr_p nstr_slice(nstr_p s, bool can_new, int32_t index, int32_t chars);
{
    void * start = NULL;
    int32_t r_bytes = 0;
    int32_t r_chars = 0;

    // CASE-1: 切片起点超出范围
    // CASE-2: 源串是空串
    // CASE-3: 切片长度是零
    // NOTE: 不生成切片，直接返回空串
    if (index >= s->chars || s->chars == 0 || chars == 0) return nstr_blank();

    // CASE-4: 源字符串不是空串
    start = get_start(s);
    if (index > 0) {
        r_chars = index;
        r_bytes = s->vtbl.count(start, s->bytes, &r_chars);
        if (r_bytes == 0) return nstr_blank(); // index 超出串尾
        if (r_bytes < 0) return NULL; // 编码不正确
        start += r_bytes;
    } // if

    r_chars = s->chars - index; // 最大切片范围是整个源串
    r_bytes = s->vtbl.count(start, s->bytes - r_bytes, &r_chars);
    if (r_bytes == 0) return nstr_blank(); // 超出串尾
    if (r_bytes < 0) return NULL; // 编码不正确

    if (can_new) return nstr_new(start, r_bytes, nstr_encoding(s));
    return new_slice(get_entity(s), start - get_start(s), r_bytes, r_chars);
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
    nstr_t prev = {0}; // 分隔符前子串切片
    nstr_t curr = {0}; // 分隔符切片

    nstr_p new = NULL; // 新子串
    void * loc = NULL; // 分隔符地址
    int32_t index = 0; // 分隔符索引
    int32_t ret = 0; // 返回值
    int rmd = 0; // 剩余切分次数，零表示停止，负数表示无限次
    int cnt = 0; // 子串数量，用于下标时始终指向下一个可用元素
    int cap = 0; // 数组容量
    int delta = 0; // 减量

    if (s->chars == 0) {
        // CASE-1: 源串是空串
        *as = malloc(sizeof(as[0]) * 2);
        if (! *as) return NULL;
        (*as)[0] = nstr_blank();
        (*as)[1] = NULL;
        return 1;
    } // if

    rmd = (max > 0) ? max : -1;
    delta = (rmd > 0) ? 1 : 0;

    // max 次分割将产生 max + 1 个子串，再加上 1 个 NULL 终止标志
    // 保留 2 个元素给最后的子串和终止标志
    cap = (rmd < 16 - 2) ? 16 : (rmd + 2);

    *as = malloc(sizeof((*as)[0]) * cap);
    if (! *as) goto NSTR_SPLIT_END;

    offset = get_offset(s);
    init_slice(&prev, STR_DONT_FREE, get_start(s), offset, 0, 0);
    init_slice(&curr, STR_DONT_FREE, get_start(s), offset, 0, 0);
    while (rmd != 0 && index < s->chars) {
        if (cnt >= cap - 2 && (ret = augment_array(as, &cap, 16)) < 0) goto NSTR_SPLIT_ERROR;

        index = nstr_next_sub(s, deli, &curr); // 如果失败，在 nstr_next_sub() 中释放 curr
        if ((ret = index) == STR_UNKNOWN_BYTE) goto NSTR_SPLIT_ERROR;
        if (index == STR_NOT_FOUND) {
            // 校准到源串尾
            loc = get_end(s);
            index = s->chars;
        } else {
            loc = get_start(curr);
        } // if

        if (can_new) {
            new = nstr_new(get_end(prev), loc - get_end(prev), nstr_encoding(s));
        } else {
            //new = new_slice(get_entity(s), get_offset(prev) + prev->bytes, sub->bytes, index - (prev->slc.index + prev->slc.chars), sub->chars);
            offset = get_offset(prev) + prev->bytes;
            new = new_slice(get_start(s) + offset, offset, sub->bytes, sub->chars);
        } // if
        if (! new) {
            nstr_delete(&curr);
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
    nstr_delete(&prev);
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

    if (cnt == 0) {
        return nstr_blank();
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

    pos = copy(new->str.data, as, n, dbuf, dbytes);

    va_copy(cp, *ap);
    while ((as2 = va_arg(cp, nstr_p *))) {
        pos = copy(pos, as2, va_args(cp, int), dbuf, dbytes);
    } // while
    va_end(cp);

    bytes -= dbytes; // 去掉多余的尾部间隔符。
    new->str.data[bytes] = 0; // 设置终止 NUL 字符。
    init_entity(new, STR_NEED_FREE, bytes, chars, as[0]->encoding);
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

    if (s->bytes == 0) return nstr_blank(); // CASE-1: s 是空串。
    if (n <= 1) return nstr_add_ref(s);

    new = calloc(1, entity_size(s->bytes * n));
    if (! new) return NULL;

    pos = copy_strings(new->str.data, as, n % (sizeof(as) / sizeof(as[0])), NULL, 0);

    b = n / (sizeof(as) / sizeof(as[0]));
    while (b-- > 0) pos = copy_strings(pos, as, (sizeof(as) / sizeof(as[0])), NULL, 0);

    init_entity(new, STR_NEED_FREE, s->bytes * n, s->chars * n, s->encoding);
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
    if (bytes == 0) return nstr_blank();

    new = calloc(1, entity_size(bytes));
    if (! new) return NULL;

    memcpy(new->str.data, get_start(s1), s1->bytes);
    memcpy(new->str.data + s1->bytes, get_start(s2), s2->bytes);
    init_entity(new, STR_NEED_FREE, bytes, s1->chars + s2->chars, s1->encoding);
    return new;
} // nstr_concat2

nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3)
{
    nstr_p new = NULL;
    int32_t bytes = 0;

    bytes = s1->bytes + s2->bytes + s3->bytes;
    if (bytes == 0) return nstr_blank();

    new = calloc(1, entity_size(bytes));
    if (! new) return NULL;

    memcpy(new->str.data, get_start(s1), s1->bytes);
    memcpy(new->str.data + s1->bytes, get_start(s2), s2->bytes);
    memcpy(new->str.data + s1->bytes + s2->bytes, get_start(s3), s3->bytes);
    init_entity(new, STR_NEED_FREE, bytes, s1->chars + s2->chars + s3->chars, s1->encoding);
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

    d->str.data[0] = deli;
    init_entity(&d, STR_DONT_FREE, 1, 1, STR_ENC_ASCII);

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
    r_bytes = s->vtbl.count(get_start(s), s->bytes, &r_chars);
    if (r_bytes == 0) return nstr_duplicate(s); // 超出串尾
    if (r_bytes < 0) return NULL; // 编码不正确

    start = origin + r_bytes;
    r_chars = chars;
    r_bytes = s->vtbl.count(start, s->bytes - r_bytes, &r_chars);
    if (r_bytes == 0) return nstr_duplicate(s); // 超出串尾
    if (r_bytes < 0) return NULL; // 编码不正确

    s1_bytes = start - origin;
    init_slice(&s1, STR_DONT_FREE, ent->str.data, offset, s1_bytes, index);
    init_slice(&s3, STR_DONT_FREE, ent->str.data, offset + s1_bytes + r_bytes, s->bytes - s1_bytes - r_bytes, s->chars - index - r_chars);
    new = nstr_concat3(s1, sub, s2);
    nstr_delete(&s3);
    nstr_delete(&s1);
    return new;
} // nstr_replace

nstr_p nstr_replace_char(nstr_p s, bool can_new, int32_t index, int32_t chars, char_t ch)
{
    nstr_t sub = {0};

    sub->str.data[0] = ch;
    init_entity(sub, STR_DONT_FREE, 1, 1, s->encoding);
    return nstr_replace(s, can_new, index, chars, &sub);
} // nstr_replace_char

nstr_p nstr_remove(nstr_p s, bool can_new, int32_t index, int32_t chars)
{
    return nstr_replace(s, can_new, index, chars, &blank);
} // nstr_remove

nstr_p nstr_cut_head(nstr_p s, bool can_new, int32_t chars)
{
    if (s->chars < chars) return nstr_blank(); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, can_new, 0, chars, &blank);
} // nstr_cut_head

nstr_p nstr_cut_tail(nstr_p s, bool can_new, int32_t chars)
{
    if (s->chars < chars) return nstr_blank(); // CASE-1: 删除长度大于字符串长度。
    return nstr_replace(s, can_new, s->chars - chars, chars, &blank);
} // nstr_cut_tail
