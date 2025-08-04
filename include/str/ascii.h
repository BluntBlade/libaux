#ifndef _AUX_ASCII_H_
#define _AUX_ASCII_H_ 1

#include "types.h"

// 测量给定位置的 ASCII 字符长度（字节数），返回 0 表示存在异常字节
inline static int32_t ascii_measure(void * pos)
{
    char_t ch = ((char_t *)pos)[0];
    return ch && (ch & 0x80 == 0) ? 1: 0;
} // ascii_measure

// 计算给定字节范围内有多少个 ASCII 字符
int32_t ascii_count(void * start, int32_t size, int32_t * chars);

// 以 start 为起点，跳过前 index 个字符（字节），检查是否存在 chars 个字符长的子串
inline static int32_t ascii_locate(void * start, int32_t size, int32_t index, int32_t * chars)
{
    int32_t r_chars = index;
    int32_t r_bytes = ascii_count(start, size, &r_chars);
    if (r_chars < index) return -1;
    return ascii_count(start + r_bytes, size - r_bytes, chars);
} // ascii_locate

// 校验给定节字范围是否完全包含正确的 ASCII 字符（除了 NUL 字符）
inline static bool ascii_verify(void * start, int32_t size)
{
    return ascii_count(start, size, NULL) == size;
} // ascii_verify

#endif // _AUX_ASCII_H_

