#ifndef _AUX_ASCII_H_
#define _AUX_ASCII_H_ 1

#include "types.h"

// 测量给定位置的 ASCII 字符长度（字节数），返回 0 表示存在异常字节
inline static int32_t ascii_measure(const char_t * pos)
{
    return (pos[0] & 0x7F) != 0;
} // ascii_measure

// 计算给定字节范围内有多少个 ASCII 字符
int32_t ascii_count(const char_t * start, int32_t size, int32_t * chars);

// 校验给定节字范围是否完全包含正确的 ASCII 字符（除了 NUL 字符）
inline static bool ascii_verify(const char_t * start, int32_t size)
{
    return ascii_count(start, size, NULL) == size;
} // ascii_verify

#endif // _AUX_ASCII_H_

