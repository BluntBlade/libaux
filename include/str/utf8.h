// UTF-8 encodes code points in one to four bytes, depending on the value of the code point. In the following table, the characters u to z are replaced by the bits of the code point, from the positions U+uvwxyz:
// Code point ↔  UTF-8 conversion 
// First Code Point    Last Code Point    Byte 1      Byte 2      Byte 3      Byte 4
// U+0000              U+007F             0yyyzzzz
// U+0080              U+07FF             110xxxyy    10yyzzzz
// U+0800              U+FFFF             1110wwww    10xxxxyy    10yyzzzz
// U+010000            U+10FFFF           11110uvv    10vvwwww    10xxxxyy    10yyzzzz

#ifndef _AUX_UTF8_H_
#define _AUX_UTF8_H_ 1

#include "types.h"

// 测量给定位置的 UTF-8 字符长度（字节数），返回 0 表示存在异常字节。
inline static uint32_t utf8_measure(void * pos)
{
    char_t ch = ((char_t *)pos)[0];
    if ((ch & 0x80) == 0) return 1;
    if (((ch <<= 1) & 0x80) == 0) return 0;
    if (((ch <<= 1) & 0x80) == 0) return 2;
    if (((ch <<= 1) & 0x80) == 0) return 3;
    if (((ch <<= 1) & 0x80) == 0) return 4;
    return 0;
} // utf8_measure

// 计算给定字节范围内有多少个 UTF-8 字符。
int32_t utf8_count(void * start, int32_t size, int32_t * chars);

// 以 start 为起点，跳过前 index 个字符，检查是否存在 chars 个字符长的子串
inline static int32_t utf8_locate(void * start, int32_t size, int32_t index, int32_t * chars)
{
    int32_t r_chars = index;
    int32_t r_bytes = utf8_count(start, size, &r_chars);
    if (r_chars < index) return -1;
    return utf8_count(start + r_bytes, size - r_bytes, chars);
} // utf8_locate

// 校验给定节字范围是否完全包含正确的 UTF-8 字符（除了 NUL 字符）
inline static bool utf8_verify(void * start, int32_t size)
{
    return utf8_count(start, size, NULL) == size;
} // if

#endif // _AUX_UTF8_H_

