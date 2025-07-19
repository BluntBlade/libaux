#ifndef _AUX_STRING_H_
#define _AUX_STRING_H_ 1

#include "types.h"

struct NSTR;
typedef struct NSTR * nstr_p;

typedef enum STR_ENCODING {
    STR_ASCII = 0,
    STR_UTF8  = 1,
    STR_ENCODING_COUNT = 2,
    STR_ENCODING_MAX = (1 << 8) - 1,  // 支持最多 255 种编码方案
} str_encoding_t;

enum {
    STR_NEED_FREE = true,
    STR_DONT_FREE = false,
};

// 需要分配内存字节数
extern size_t nstr_object_size(uint32_t bytes);

// 从原生字符串生成新字符串
extern nstr_p nstr_new(void * src, uint32_t bytes, str_encoding_t encoding);

// 从源字符串（或片段引用）生成新字符串
extern nstr_p nstr_clone(nstr_p s);

// 删除字符串
extern void nstr_delete(nstr_p * ps);

// 删除切分后的字符串数组
extern void nstr_delete_strings(nstr_p * as, int n);

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

// 返回原生字符串指针（片段引用情况下会生成一个新字符串）
extern void * nstr_to_cstr(nstr_p * ps);

// 返回片段引用数（返回 0 表示这是一个片段引用）
extern uint32_t nstr_refs(nstr_p s);

// 测试是否为字符串
extern bool nstr_is_str(nstr_p s);

// 测试是否为片段引用
extern bool nstr_is_ref(nstr_p s);

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

// 校验编码正确性和完整性
extern bool nstr_verify(nstr_p s);

// 查找子串
extern void * nstr_find(nstr_p s, nstr_p sub, void ** pos, void ** end, uint32_t * bytes);

// 初始化遍历字节
extern void * nstr_first_byte(nstr_p s, void ** pos, void ** end);

// 遍历字节
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

// 初始化遍历字符
extern void * nstr_first_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes);

// 遍历字符
extern void * nstr_next_char(nstr_p s, void ** pos, void ** end, uint32_t * bytes);

// 获取空字符串常量
extern nstr_p nstr_blank(str_encoding_t encoding);

// 生成（字符范围的）片段引用，或新字符串
extern nstr_p nstr_slice(nstr_p s, bool no_ref, uint32_t index, uint32_t chars);

// 生成（字节范围的）片段引用，或新字符串
extern nstr_p nstr_slice_from(nstr_p s, bool no_ref, void * pos, uint32_t bytes);

// 切分字符串
extern nstr_p * nstr_split(nstr_p deli, nstr_p s, bool no_ref, int * max);

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

// 在给定位置插入子串
extern nstr_p nstr_insert(nstr_p s, uint32_t index, nstr_p sub);

// 在给定位置插入单字节字符
extern nstr_p nstr_insert_char(nstr_p s, uint32_t index, char_t ch);

// 在串头前插入子串
extern nstr_p nstr_prepend(nstr_p s, nstr_p sub);

// 在串头前插入单字节字符
extern nstr_p nstr_prepend_char(nstr_p s, char_t ch);

// 在串尾后插入子串
extern nstr_p nstr_append(nstr_p s, nstr_p sub);

// 在串尾后插入单字节字符
extern nstr_p nstr_append_char(nstr_p s, char_t ch);

// 将给定位置处的固定长度子串替换成新串
extern nstr_p nstr_replace(nstr_p s, uint32_t index, uint32_t chars, nstr_p sub);

// 将给定位置处的固定长度子串替换成单字节字符
extern nstr_p nstr_replace_char(nstr_p s, uint32_t index, uint32_t chars, char_t ch);

// 将子串替换成新串
extern nstr_p nstr_substitue(nstr_p s, bool all, nstr_p before, nstr_p after);

// 将子串替换成单字节字符
extern nstr_p nstr_substitue_char(nstr_p s, bool all, nstr_p sub, char_t ch);

// 删除定位置处的固定长度子串
extern nstr_p nstr_remove(nstr_p s, bool no_ref, uint32_t index, uint32_t chars);

// 删除串头的固定长度子串
extern nstr_p nstr_cut_head(nstr_p s, bool no_ref, uint32_t chars);

// 删除串尾的固定长度子串
extern nstr_p nstr_cut_tail(nstr_p s, bool no_ref, uint32_t chars);

// 删除串尾的换行符（以及可能的回车符）
extern nstr_p nstr_chomp(nstr_p s, bool no_ref);

// 删除串头和串尾的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_trim(nstr_s, bool no_ref);

// 删除串头的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_ltrim(nstr_s, bool no_ref);

// 删除串尾的空白字符（SPACE/TAB/CR/NL等）
extern nstr_p nstr_rtrim(nstr_s, bool no_ref);

#endif // _AUX_STRING_H_

