#ifndef _AUX_STRING_H_
#define _AUX_STRING_H_ 1

#include <assert.h>
#include <string.h>

#include "types.h"

struct NSTR;
typedef struct NSTR * nstr_p;
typedef nstr_p * nstr_array_p;

typedef enum STR_ENCODING {
    STR_ENC_ASCII = 0,
    STR_ENC_UTF8  = 1,
    STR_ENC_UTF16 = 2,
    STR_ENC_COUNT,
    STR_ENC_MAX = (1 << 6),     // 支持最多 64 种编码方案
} str_encoding_t;

enum {
    STR_OUT_OF_MEMORY = -1,
    STR_UNKNOWN_BYTE = -2,
    STR_NOT_FOUND = -101,
};

typedef enum STR_LOCALE {
    STR_LOC_C = 0,
} str_locale_t;

// ---- 功能函数 ---- //

// 引用一个新串
extern nstr_p nstr_new(const char_t * src, uint32_t bytes, bool copy);

// 引用一个新空串
extern nstr_p nstr_new_blank(str_encoding_t encoding);

// 引用外部字节范围
inline static nstr_p nstr_new_reference(const char_t * src)
{
    assert(src);
    return nstr_new(src, strlen((void *)src), false);
} // nstr_new_reference

// 复制源串（深拷贝）
extern nstr_p nstr_clone(nstr_p s);

// 复制切片（浅拷贝）
extern nstr_p nstr_duplicate(nstr_p s);

// 删除字符串
extern void nstr_delete(nstr_p s);

// 删除切分后的字符串数组
extern void nstr_delete_strings(nstr_p * as, int n);

// 返回编码方案代号
extern str_encoding_t nstr_encoding(nstr_p s);

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

// 测试是否为空字符串
inline static bool nstr_is_blank(nstr_p s)
{
    return nstr_chars(s) == 0;
} // nstr_is_blank

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

// 功能：获取字符数据区的起止地址
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     start  IO    入参：遍历状态变量的指针，不能为 NULL
//                  出参：起始地址
//     end    IO    入参：遍历状态变量的指针，不能为 NULL
//                  出参：终止地址
// 返回值：
//     无
extern void nstr_byte_range(nstr_p s, const char_t ** start, const char_t ** end);

// 功能：获取下一字符
// 参数：
//     s      IN    入参：源串或切片，指向一个非零长度的串
//     sub    OUT   出参：下一字符的切片，引用其在源串中的正确位置
// 返回值：
//     0 <          跳过字节数，累加可得字节下标
//     == 0         没有更多字符，遍历结束
extern uint32_t nstr_next_char(nstr_p s, const char_t ** start, uint32_t * index, nstr_p ch);

// 功能：查找子串
// 参数：
//     s      IN    入参：源串或切片，指向一个非零长度的串
//     sub    IO    入参：目标子串，指向一个非零长度的切片
//                  出参：目标子串，引用其在源串中的正确位置
// 返回值：
//     >= 0                 目标子串前的字符数，累加可得子串下标
//     STR_NOT_FOUND        没有找到子串，查找结束
//     STR_UNKNOWN_BYTE     源串包含异常字节（未正确编码）
// 说明：
//     本函数在源串中查找子串，并修改子串的引用位置。查找结束后，如再次以相同对象调用，则会绕回到源串开头，启动新一轮查找。
extern uint32_t nstr_next_sub(nstr_p s, nstr_p sub, const char_t ** start, uint32_t * index);

#define nstr_find next_next_sub

// 设置编码
extern bool nstr_set_encoding(nstr_p s, str_encoding_t encoding);

// 收窄切片范围
extern void nstr_narrow_down(nstr_p s, uint32_t index, uint32_t chars);

// 基于字符范围，生成或设置切片
extern nstr_p nstr_slice(nstr_p s, uint32_t index, uint32_t chars, nstr_p r);

// 功能：切分字符串
// 参数：
//     s        IN  入参：源字符串，必须传入 non-NULL
//     deli     IN  入参：分隔符，NULL 表示将每个字符切分成单独字符串
//     max      IO  入参：最大切分次数，NULL 或 < 0 表示无限次
//                  出参：子串数量
// 返回值：
//     non-NULL     指针数组，包含 max 个子串和 1 个 NULL 终止标志，共 max + 1 个元素
//     NULL         发生错误，内存不足
extern int nstr_split(nstr_p s, nstr_p deli, int max, nstr_array_p * as);

// 重复拼接字符串
extern nstr_p nstr_repeat(nstr_p s, int n, nstr_p r);

// 拼接字符串
extern nstr_p nstr_concat(nstr_p * as, int n, nstr_p r, ...);

// 拼接两个字符串
extern nstr_p nstr_concat2(nstr_p s1, nstr_p s2, nstr_p r);

// 拼接三个字符串
extern nstr_p nstr_concat3(nstr_p s1, nstr_p s2, nstr_p s3, nstr_p r);

// 拼接多个字符串，以给定字符串分隔
extern nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, nstr_p r, ...);

// 拼接多个字符串，以给定单字节字符分隔
extern nstr_p nstr_join_by_char(char_t deli, nstr_p * as, int n, nstr_p r, ...);

// 拼接两个字符串，以给定字符串分隔
inline static nstr_p nstr_join2(nstr_p deli, nstr_p s1, nstr_p s2, nstr_p r)
{
    return nstr_concat3(s1, deli, s2, r);
} // nstr_join2

// 将给定位置处的固定长度子串替换成新串
extern nstr_p nstr_replace(nstr_p s, uint32_t index, uint32_t chars, nstr_p to, nstr_p r);

// 将给定位置处的固定长度子串替换成单字节字符
extern nstr_p nstr_replace_with_char(nstr_p s, uint32_t index, uint32_t chars, char_t ch, nstr_p r);

// 在给定位置插入子串
inline static nstr_p nstr_insert(nstr_p s, uint32_t index, nstr_p sub, nstr_p r)
{
    return nstr_replace(s, index, 0, sub, r);
} // nstr_insert

// 在给定位置插入单字节字符
inline static nstr_p nstr_insert_char(nstr_p s, uint32_t index, char_t ch, nstr_p r)
{
    return nstr_replace_with_char(s, index, 0, ch, r);
} // nstr_insert_char

// 在串头前插入子串
inline static nstr_p nstr_prepend(nstr_p s, nstr_p sub, nstr_p r)
{
    return nstr_replace(s, 0, 0, sub, r);
} // nstr_prepend

// 在串头前插入单字节字符
inline static nstr_p nstr_prepend_char(nstr_p s, char_t ch, nstr_p r)
{
    return nstr_replace_with_char(s, 0, 0, ch, r);
} // nstr_prepend_char

// 在串尾后插入子串
inline static nstr_p nstr_append(nstr_p s, nstr_p sub, nstr_p r)
{
    return nstr_replace(s, nstr_chars(s), 0, sub, r);
} // nstr_append

// 在串尾后插入单字节字符
inline static nstr_p nstr_append_char(nstr_p s, char_t ch, nstr_p r)
{
    return nstr_replace_with_char(s, nstr_chars(s), 0, ch, r);
} // nstr_append_char

// 删除定位置处的固定长度子串
extern nstr_p nstr_remove(nstr_p s, uint32_t index, uint32_t chars, nstr_p r);

// 删除串头的固定长度子串
extern nstr_p nstr_cut_head(nstr_p s, uint32_t chars, nstr_p r);

// 删除串尾的固定长度子串
extern nstr_p nstr_cut_tail(nstr_p s, uint32_t chars, nstr_p r);

// 删除串尾的换行符（以及可能的回车符）
extern nstr_p nstr_chomp(nstr_p s, nstr_p r);

// 删除串头和串尾的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_trim(nstr_p s, nstr_p r);

// 删除串头的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_ltrim(nstr_p s, nstr_p r);

// 删除串尾的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_rtrim(nstr_p s, nstr_p r);

// 替换子串
extern nstr_p nstr_substitute(nstr_p s, bool all, nstr_p from, nstr_p to, nstr_p r);

#endif // _AUX_STRING_H_

