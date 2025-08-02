#ifndef _AUX_STRING_H_
#define _AUX_STRING_H_ 1

#include "types.h"

struct NSTR;
typedef struct NSTR * nstr_p;
typedef nstr_p * nstr_array_p;

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

enum {
    STR_OUT_OF_MEMORY = -1,
    STR_UNKNOWN_BYTE = -2,
    STR_NOT_FOUND = -101,
    STR_BLANK_SUB = -102,
};

typedef enum STR_LOCALE {
    STR_LOC_C = 0,
} str_locale_t;

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

// 测试是否为空字符串
extern bool nstr_is_blank(nstr_p s);

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

// 功能：获取串内容首字节的地址
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     pos    IO    入参：遍历状态变量的指针，不能为 NULL
//                  出参：将遍历的下一字节地址
//     end    IO    入参：遍历状态变量的指针，不能为 NULL
//                  出参：遍历终止地址
// 返回值：
//     non-NULL     首字节地址
//     NULL         没有首字节，遍历结束
extern void * nstr_first_byte(nstr_p s, void ** pos, void ** end);

// 功能：获取串内容下一字节的地址
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     pos    IO    入参：遍历状态变量的指针，不能为 NULL
//                  出参：将遍历的下一字节地址
//     end    IO    入参：遍历状态变量的指针，不能为 NULL
//                  出参：遍历终止地址
// 返回值：
//     non-NULL     下一字节地址
//     NULL         没有下一字节，遍历结束
inline static void * nstr_next_byte(nstr_p s, void ** pos, void ** end)
{
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        return NULL;
    } // if
    return *pos++;
} // nstr_next_byte

// 功能：获取串内容首字符
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     slice  IO    入参：遍历状态变量的指针，不能为 NULL
//                        允许指向外部分配的内存，或内部分配内存（结束时释放），以容纳临时切片对象
//                  出参：切片对象，引用单个字符
// 返回值：
//     >= 0         字符下标
//     -1           没有更多字符，遍历结束
//     -2           源字符串包含异常字节（未正确编码）
//     -3           无充足内存可用
extern int32_t nstr_first_char(nstr_p s, nstr_p * slice);

// 功能：获取串内容下一字符
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     slice  IO    入参：遍历状态变量的指针，不能为 NULL
//                        允许指向外部分配的内存，或内部分配内存（结束时释放），以容纳临时切片对象
//                  出参：切片对象，引用单个字符
// 返回值：
//     > 0          字符下标
//     -1           没有更多字符，遍历结束
//     -2           源字符串包含异常字节（未正确编码）
extern nstr_p nstr_next_char(nstr_p s, nstr_p * slice);

// 功能：查找子串首个位置
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     sub    IN    入参：目标子串或切片，不能为 NULL
//     slice  IO    入参：遍历状态变量的指针，不能为 NULL
//                        允许指向外部分配的内存，或内部分配内存（结束时释放），以容纳临时切片对象
//                  出参：切片对象，引用找到的子串
// 返回值：
//     >= 0                 子串位置下标
//     STR_OUT_OF_MEMORY    内存不足
//     STR_UNKNOWN_BYTE     源串包含异常字节（未正确编码）
//     STR_NOT_FOUND        没有找到子串，查找结束
extern int32_t nstr_first_sub(nstr_p s, nstr_p sub, nstr_p * slice);

// 功能：查找子串下一位置
// 参数：
//     s      IN    入参：源串或切片，不能为 NULL
//     sub    IN    入参：目标子串或切片，不能为 NULL
//     slice  IO    入参：遍历状态变量的指针，不能为 NULL
//                        允许指向外部分配的内存，或内部分配内存（结束时释放），以容纳临时切片对象
//                  出参：切片对象，引用找到的子串
// 返回值：
//     >= 0                 子串位置下标
//     STR_UNKNOWN_BYTE     源串包含异常字节（未正确编码）
//     STR_NOT_FOUND        没有找到子串，查找结束
extern int32_t nstr_next_sub(nstr_p s, nstr_p sub, nstr_p * slice);

// 查找首个或下一子串
inline static int32_t nstr_find(nstr_p s, nstr_p sub, nstr_p * slice)
{
    return (*slice) ? nstr_next_sub(s, sub, slice) : nstr_first_sub(s, sub, slice);
} // nstr_find

// 获取空字符串常量
extern nstr_p nstr_blank(void);

// 基于字符范围，生成切片或新字符串
extern nstr_p nstr_slice(nstr_p s, bool can_new, uint32_t index, uint32_t chars);

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
extern int nstr_split(nstr_p s, bool can_new, nstr_p deli, int max, nstr_array_p * as);

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

