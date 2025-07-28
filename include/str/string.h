#ifndef _AUX_STRING_H_
#define _AUX_STRING_H_ 1

#include "types.h"

struct NSTR;
typedef struct NSTR * nstr_p;

typedef enum STR_ENCODING {
    STR_ENC_ASCII = 0,
    STR_ENC_UTF8  = 1,
    STR_ENC_COUNT = 2,
    STR_ENC_MAX = (1 << 8) - 1,  // 支持最多 255 种编码方案
} str_encoding_t;

enum {
    STR_NEED_FREE = true,
    STR_DONT_FREE = false,
    STR_AND_NEW = true,
    STR_NOT_NEW = false,
};

typedef enum STR_LOCALE {
    STR_LOC_C = 0,
} str_locale_t;

// ---- 元函数 ---- //

// 功能：返回切片对象应占字节数，以便预分配内存（并嵌入到其它数据结构中）
// 参数：
//     无
// 返回值：
//     > 0    切片对象需要的固定大小的内存字节数，调用方自行圆整并对齐到内存边界
extern size_t nstr_slice_size(void);

// 初始化切片对象
extern void nstr_init_slice(nstr_p s, bool need_free, nstr_p src, uint32_t offset, uint32_t bytes, uint32_t chars);

// ---- 功能函数 ---- //

// 从原生字符串生成新字符串
extern nstr_p nstr_new(void * src, uint32_t bytes, str_encoding_t encoding);

// 从源字符串（或切片）生成新字符串
extern nstr_p nstr_clone(nstr_p s);

// 删除字符串
extern void nstr_delete(nstr_p * ps);

// 删除切分后的字符串数组
extern void nstr_delete_strings(nstr_p * as, int n);

// 增加引用计数
extern nstr_p nstr_add_ref(nstr_p s);

// 减少引用计数
inline static nstr_p nstr_del_ref(nstr_p * ps)
{
    nstr_delete(ps);
} // nstr_del_ref

// 返回编码方案代号
extern uint32_t nstr_encoding(nstr_p s);

// 返回字节数，不包含最后的 NUL 字符
extern uint32_t nstr_bytes(nstr_p s);

inline static uint32_t nstr_size(nstr_p s)
{
    return nstr_bytes(s);
} // nstr_size

// 返回字符数，不包含最后的 NUL 字符
extern uint32_t nstr_chars(nstr_p s);

inline static uint32_t nstr_length(nstr_p s)
{
    return nstr_chars(s);
} // nstr_length

// 返回原生字符串指针（切片情况下会生成一个新字符串）
extern void * nstr_to_cstr(nstr_p * ps);

// 返回切片数（返回 0 表示这是一个切片）
extern uint32_t nstr_refs(nstr_p s);

// 测试是否为字符串
extern bool nstr_is_string(nstr_p s);

// 测试是否为切片
extern bool nstr_is_slice(nstr_p s);

// 测试是否存在子串
extern bool nstr_contain(nstr_p s, nstr_p sub);

// 测试是否存在单字节字符
extern bool nstr_contain_char(nstr_p s, char_t ch);

// 测试串头是否为给定子串
extern bool nstr_start_with(nstr_p s, nstr_p sub);

// 测试串头是否为给定单字节字符
extern bool nstr_start_with_char(nstr_p s, char_t ch);

// 测试串尾是否为给定子串
extern bool nstr_end_with(nstr_p s, nstr_p sub);

// 测试串尾是否为给定单字节字符
extern bool nstr_end_with_char(nstr_p s, char_t ch);

// 比较字符串（排序）
extern int nstr_compare(nstr_p s1, nstr_p s2, str_locale_t locale);

inline static bool nstr_equal(nstr_p s1, nstr_p s2, str_locale_t locale)
{
    return nstr_compare(s1, s2, locale) == 0;
} // nstr_equal

inline static bool nstr_less(nstr_p s1, nstr_p s2, str_locale_t locale)
{
    return nstr_compare(s1, s2, locale) < 0;
} // nstr_less

inline static bool nstr_less_or_equal(nstr_p s1, nstr_p s2, str_locale_t locale)
{
    return nstr_compare(s1, s2, locale) <= 0;
} // nstr_less_or_equal

inline static bool nstr_greater(nstr_p s1, nstr_p s2, str_locale_t locale)
{
    return nstr_compare(s1, s2, locale) > 0;
} // nstr_greater

inline static bool nstr_greater_or_equal(nstr_p s1, nstr_p s2, str_locale_t locale)
{
    return nstr_compare(s1, s2, locale) >= 0;
} // nstr_greater_or_equal

// 校验编码正确性和完整性
extern bool nstr_verify(nstr_p s);

// 获取第一个字节
extern void * nstr_first_byte(nstr_p s, void ** pos, void ** end);

// 获取下一个字节
inline static void * nstr_next_byte(nstr_p s, void ** pos, void ** end)
{
    void * loc = *pos;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        return NULL;
    } // if
    *pos += 1;
    return loc;
} // nstr_next_byte

// 获取第一个字符
extern void * nstr_first_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes);

// 获取下一个字符
extern void * nstr_next_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes);

// 定位第一个子串位置
extern void * nstr_first_sub(nstr_p s, nstr_p sub, void ** start, uint32_t * size, uint32_t * index);

// 定位下一个子串位置
extern void * nstr_next_sub(nstr_p s, nstr_p sub, void ** start, uint32_t * size, uint32_t * index);

// 查找第一个或下一个子串
inline static void * nstr_find(nstr_p s, nstr_p sub, void ** start, uint32_t * size, uint32_t * index)
{
    if (! *start) {
        return nstr_first_sub(s, sub, start, size, index);
    } // if
    return nstr_next_sub(s, sub, start, size, index);
} // nstr_find

// 获取空字符串常量
extern nstr_p nstr_blank(str_encoding_t encoding);

// 基于字符范围切片，生成切片或新字符串
extern nstr_p nstr_slice(nstr_p s, bool can_new, uint32_t index, uint32_t chars);

// 基于字字范围切片，生成切片或新字符串
extern nstr_p nstr_slice_from(nstr_p s, bool can_new, void * pos, uint32_t bytes);

// 功能：切分字符串
// 参数：
//     s        IN  入参：源字符串，必须传入 non-NULL
//     can_new  IN  入参：true 表示生成新字符串；false 表示生成切片
//     deli     IN  入参：分隔符，NULL 表示将每个字符切分成单独字符串
//     max      IO  入参：最大切分次数，NULL 或 < 0 表示无限次
//                  出参：子串数量
// 返回值：
//     non-NULL     指针数组，包含 max 个子串和 1 个 NULL 终止标志，共 max + 1 个元素
//     NULL         发生错误，内存不足
extern nstr_p * nstr_split(nstr_p s, bool can_new, nstr_p deli, int * max);

// 重复拼接字符串
extern nstr_p nstr_repeat(nstr_p s, int n);

// 拼接字符串
extern nstr_p nstr_concat(nstr_p * as, int n, ...);

// 拼接两个字符串
extern nstr_p nstr_concat2(nstr_p s1, nstr_p s2);

// 拼接三个字符串
extern nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3);

// 拼接多个字符串，以给定字符串分隔
extern nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, ...);

// 拼接多个字符串，以给定单字节字符分隔
extern nstr_p nstr_join_with_char(char_t deli, nstr_p * as, int n, ...);

// 拼接两个字符串，以给定字符串分隔
inline static nstr_p nstr_join2(nstr_p deli, nstr_p s1, nstr_p s2)
{
    return nstr_concat3(s1, deli, s2);
} // nstr_join2

// 将给定位置处的固定长度子串替换成新串
extern nstr_p nstr_replace(nstr_p s, bool can_new, uint32_t index, uint32_t chars, nstr_p sub);

// 将给定位置处的固定长度子串替换成单字节字符
extern nstr_p nstr_replace_char(nstr_p s, bool can_new, uint32_t index, uint32_t chars, char_t ch);

// 将子串替换成新串
extern nstr_p nstr_substitue(nstr_p s, bool all, nstr_p before, nstr_p after);

// 将子串替换成单字节字符
extern nstr_p nstr_substitue_char(nstr_p s, bool all, nstr_p sub, char_t ch);

// 在给定位置插入子串
inline static nstr_p nstr_insert(nstr_p s, bool can_new, uint32_t index, nstr_p sub)
{
    return nstr_replace(s, can_new, index, 0, sub);
} // nstr_insert

// 在给定位置插入单字节字符
inline static nstr_p nstr_insert_char(nstr_p s, bool can_new, uint32_t index, char_t ch)
{
    return nstr_replace_char(s, can_new, index, 0, ch);
} // nstr_insert_char

// 在串头前插入子串
inline static nstr_p nstr_prepend(nstr_p s, bool can_new, nstr_p sub)
{
    return nstr_replace(s, can_new, 0, 0, sub);
} // nstr_prepend

// 在串头前插入单字节字符
inline static nstr_p nstr_prepend_char(nstr_p s, bool can_new, char_t ch)
{
    return nstr_replace_char(s, can_new, 0, 0, ch);
} // nstr_prepend_char

// 在串尾后插入子串
inline static nstr_p nstr_append(nstr_p s, bool can_new, nstr_p sub)
{
    return nstr_replace(s, can_new, nstr_chars(s), 0, sub);
} // nstr_append

// 在串尾后插入单字节字符
inline static nstr_p nstr_append_char(nstr_p s, bool can_new, char_t ch)
{
    return nstr_replace_char(s, can_new, nstr_chars(s), 0, ch);
} // nstr_append_char

// 删除定位置处的固定长度子串
extern nstr_p nstr_remove(nstr_p s, bool can_new, uint32_t index, uint32_t chars);

// 删除串头的固定长度子串
extern nstr_p nstr_cut_head(nstr_p s, bool can_new, uint32_t chars);

// 删除串尾的固定长度子串
extern nstr_p nstr_cut_tail(nstr_p s, bool can_new, uint32_t chars);

// 删除串尾的换行符（以及可能的回车符）
extern nstr_p nstr_chomp(nstr_p s, bool can_new);

// 删除串头和串尾的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_trim(nstr_s, bool can_new);

// 删除串头的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_ltrim(nstr_s, bool can_new);

// 删除串尾的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_rtrim(nstr_s, bool can_new);

#endif // _AUX_STRING_H_

