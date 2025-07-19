#ifndef _AUX_ASCII_H_
#define _AUX_ASCII_H_ 1

#include "types.h"

// 测量给定位置的 ASCII 字符长度（字节数），返回 0 表示存在异常字节。
inline static uint32_t ascii_measure(void * pos)
{
    char_t ch = ((char_t *)pos)[0];
    return ch && (ch & 0x80 == 0) ? 1: 0;
} // ascii_measure

// 检查 [index:index + chars] 是否为正确的 ASCII 子串（chars 给出最大长度），返回其地址和长度。如范围内存在异常字节，返回 NULL 和 0 。
extern void * ascii_check(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes);

// 计算给定字节范围内有多少个 ASCII 字符。
inline static uint32_t ascii_count(void * begin, void * end)
{
    uint32_t chars = 0;
    ascii_check(begin, end, 0, &chars);
    return chars;
} // ascii_count

// 校验给定节字范围是否完全包含正确的 ASCII 字符（除了 NUL 字符）。
inline static bool ascii_verify(void * begin, void * end)
{
    uint32_t chars = 0;
    return ascii_check(begin, end, 0, &chars) != NULL;
} // ascii_verify

#endif // _AUX_ASCII_H_

