#ifndef _AUX_ASCII_H_
#define _AUX_ASCII_H_ 1

#include "types.h"

// 测量给定位置的 ASCII 字符长度（字节数），返回 0 表示存在异常字节
inline static uint32_t ascii_measure(const char_t * pos)
{
    return ((pos[0] >> 6) | !!pos[0]) == 1; // 等价于  0 < pos[0] && pos[0] <= 0x7F
} // ascii_measure

// 计算给定字节范围内有多少个 ASCII 字符
bool ascii_count_plain(const char_t * start, uint32_t * bytes, uint32_t * chars);

// 计算给定字节范围内有多少个 ASCII 字符（加速版）
bool ascii_count_unroll(const char_t * start, uint32_t * bytes, uint32_t * chars);

#define ascii_count ascii_count_unroll

#endif // _AUX_ASCII_H_

