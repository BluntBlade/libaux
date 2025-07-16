#ifndef _NSTR_H_
#define _NSTR_H_ 1

struct NSTR;
typedef struct NSTR * nstr_p;

// 需要分配内存字节数
size_t nstr_should_allocate(uint64_t size);

// 从原生字符串生成新字符串
nstr_p nstr_new_from_cstr(const char * src, uint64_t size, uint64_t code);
nstr_p nstr_new_from_wstr(const wchar_t * src, uint64_t size, uint64_t code);

// 删除字符串
void nstr_delete(nstr_p * s);

// 返回编码方案代号
uint64_t nstr_code(nstr_p s);

// 返回内存占用字节数
uint64_t nstr_size(nstr_p s);

// 返回字符数
uint64_t nstr_length(nstr_p s);

// 返回原生字符串指针（片段引用情况下会生成一个新字符串）
const char * nstr_to_cstr(nstr_p * s);
const wchar_t * nstr_to_wstr(nstr_p * s);

// 返回片段引用数（0 代表这是一个片段引用）
uint64_t nstr_count_ref(nstr_p s);

// 增加片段引用
nstr_p nstr_add_ref(nstr_p s, uint64_t begin, uint64_t end);

// 减少片段引用
void nstr_del_ref(nstr_p * s);

// 合并多个字符串
nstr_p nstr_concat(nstr_p * s, int n, ...);

// 使用间隔符，合并多个字符串
nstr_p nstr_join(nstr_p deli, nstr_p * s, int n, ...);

// 使用间隔符，分割字符串
int nstr_split(nstr_p deli, nstr_p s, int max, nstr_p * r);

// 搜索子字符串
int64_t nstr_find(nstr_p s, uint64_t begin, nstr_p sub);

#endif // _NSTR_H_

