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

// 功能：检查 [index:index + chars] 是否包含正确的 UTF-8 子串，返回其地址和长度。
// 参数：
//     begin     IN   入参：检查范围起点
//     end       IN   入参：检查范围终点
//     index     IN   入参：从起点开始计算的字符索引
//     chars     IO   入参：子串最长字符数；出参：子串实长字符数
//     bytes     OUT  出参：子串占用字节数
// 返回值：
//     non-NULL       小于 end ：包含正确子串，chars 为子串实长字符数，bytes 为子串占用字节数
//                    等于 end ：没有包含子串，chars 为 0 ，bytes 为 0
//     NULL           给定范围内存在异常字节，chars 为 0 ，bytes 为 0
extern void * utf8_check(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes);

// 计算给定字节范围内有多少个 UTF-8 字符。
inline static uint32_t utf8_count(void * begin, void * end)
{
    uint32_t chars = end - begin; // UTF-8 字符数必然少于或等于字节数。
    check_utf8(begin, end, 0, &chars);
    return chars;
} // utf8_count

// 校验给定节字范围是否完全包含正确的 UTF-8 字符（除了 NUL 字符）。
inline static bool utf8_verify(void * begin, void * end)
{
    uint32_t chars = end - begin; // UTF-8 字符数必然少于或等于字节数。
    return check_utf8(begin, end, 0, &chars) != NULL;
} // if

#endif // _AUX_UTF8_H_

