#ifndef _AUX_STRING_H_
#define _AUX_STRING_H_ 1

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
extern size_t nstr_object_bytes(uint64_t bytes);

// 从原生字符串生成新字符串
extern nstr_p nstr_new(void * src, uint64_t bytes, str_encoding_t encoding);

// 从源字符串（或片段引用）生成新字符串
extern nstr_p nstr_clone(nstr_p s);

// 删除字符串
extern void nstr_delete(nstr_p * ps);

// 删除切分后的字符串数组
extern void nstr_delete_all(nstr_p * as, int n);

// 返回编码方案代号
extern uint64_t nstr_encoding(nstr_p s);

// 返回字节数，不包含最后的 NUL 字符
extern uint64_t nstr_bytes(nstr_p s);

inline static uint64_t nstr_size(nstr_p s)
{
    return nstr_bytes(s);
} // nstr_size

// 返回字符数，不包含最后的 NUL 字符
extern uint64_t nstr_chars(nstr_p s);

inline static uint64_t nstr_length(nstr_p s)
{
    return nstr_chars(s);
} // nstr_length

// 返回原生字符串指针（片段引用情况下会生成一个新字符串）
extern void * nstr_to_cstr(nstr_p * ps);

// 返回片段引用数（返回 0 表示这是一个片段引用）
extern uint64_t nstr_refs(nstr_p s);

// 测试是否为字符串
extern bool nstr_is_str(nstr_p s);

// 测试是否为片段引用
extern bool nstr_is_ref(nstr_p s);

// 校验完整性
extern bool nstr_verify(nstr_p s);

// 生成片段引用，或生成新字符串（字符范围）
extern nstr_p nstr_slice_chars(nstr_p s, bool no_ref, uint64_t index, uint64_t chars);

// 生成片段引用，或生成新字符串（字节范围）
extern nstr_p nstr_slice_bytes(nstr_p s, bool no_ref, void * pos, uint64_t bytes);

// 合并多个字符串
extern nstr_p nstr_concat(nstr_p * as, int n, ...);

// 使用间隔符，合并多个字符串
extern nstr_p nstr_join(nstr_p deli, nstr_p * as, int n, ...);

// 使用间隔符，分割字符串
extern nstr_p * nstr_split(nstr_p deli, nstr_p s, bool no_ref, int * max);

// 搜索子字符串
extern void * nstr_find(nstr_p s, nstr_p sub, void ** pos, void ** end, uint64_t * bytes);

// 重新编码
extern nstr_p nstr_recode(nstr_p s, str_encoding_t encoding);

// 初始化遍历
extern bool nstr_first(nstr_p s, bool by_char, void ** pos, void ** end, uint64_t * bytes);

// 遍历字节
inline static bool nstr_next_byte(nstr_p s, void ** pos, void ** end)
{
    *pos += 1;
    if (*pos == *end) {
        *pos = NULL;
        *end = NULL;
        return false;
    } // if
    return true;
} // nstr_next_byte

// 遍历字符
extern bool nstr_next_char(nstr_p s, void ** pos, void ** end, uint64_t * bytes);

// 返回空字符串
extern nstr_p nstr_blank(str_encoding_t encoding);

#endif // _AUX_STRING_H_

