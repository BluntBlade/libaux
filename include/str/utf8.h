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

extern uint8_t utf8_map[128];

// 功能：测量单个 UTF-8 字符包含的字节数
// 参数：
//     pos      IN  字符串指针，不能为 NULL
// 返回值：
//     0 <          字节数
//     0            首字节错误
//     < 0          存在异常字节
// 说明：
//     逐个检查字节，直观但较慢的逻辑。
inline static int32_t utf8_measure(const char_t * pos)
{
    char_t ch = pos[0];
    if ((ch & 0x80) == 0) return 1;
    if (((ch <<= 1) & 0x80) == 0) return 0;
    if (((ch <<= 1) & 0x80) == 0) return 2;
    if (((ch <<= 1) & 0x80) == 0) return 3;
    if (((ch <<= 1) & 0x80) == 0) return 4;
    return -1;
} // utf8_measure

// 功能：测量单个 UTF-8 字符包含的字节数（查表法）
// 参数：
//     pos      IN  字符串指针，不能为 NULL
// 返回值：
//     0 <          字节数
//     0            首字节错误
//     < 0          存在异常字节
// 说明：
//     逻辑最简单，需加载表格到缓存中，会影响性能。
inline static int32_t utf8_measure_by_lookup(const char_t * pos)
{
    return (utf8_map[pos[0] / 2] >> ((pos[0] & 0x1) * 4)) & 0x7;
} // utf8_measure_by_lookup

// 功能：测量单个 UTF-8 字符包含的字节数（累加法）
// 参数：
//     pos      IN  字符串指针，不能为 NULL
// 返回值：
//     0 <          字节数
//     0            首字节错误
//     < 0          存在异常字节
// 说明：
//     平衡机器码字节数、缓存命中率和计算性能的逻辑实现。
inline static int32_t utf8_measure_by_addup(const char_t * pos)
{
    int32_t ena = 0;    // 累加开关
    int32_t bytes = 1;  // 字节数

    bytes -= (ena  = (pos[0] >> 7));            // 首字节可能以 0b10 打头，预先减 1
    bytes += (ena &= (pos[0] >> 6)) * 2;        // 2 字节
    bytes += (ena &= (pos[0] >> 5));            // 3 字节
    bytes += (ena &= (pos[0] >> 4));            // 4 字节
    return bytes - (ena & (pos[0] >> 3)) * 5;   // 首字节是 0b11111xxx ，返回 -1
} // utf8_measure_by_addup

#define utf8_measure utf8_measure_by_addup

// 计算给定字节范围内有多少个 UTF-8 字符
int32_t utf8_count(const char_t * start, int32_t size, int32_t * chars);

// 校验给定节字范围是否完全包含正确的 UTF-8 字符（除了 NUL 字符）
inline static bool utf8_verify(const char_t * start, int32_t size)
{
    int32_t chars = size;
    return utf8_count(start, size, &chars) == size;
} // if

inline static uchar_t utf8_decode(const char_t * pos)
{
    uchar_t cpt = 0;    // Unicode 码点
    int32_t ena = 0;    // 操作开关
    int32_t bytes = 0;  // 字节数

    if ((ena = pos[0] >> 7)) {
        if ((ena &= pos[0] >> 6) == 0) return ~0L; // 返回明显错误的码点
        bytes += ena;
        cpt = (cpt << (ena * 6)) | (ena * (pos[1] & 0x3F)); // 2 字节

        ena &= pos[0] >> 5;
        bytes += ena;
        cpt = (cpt << (ena * 6)) | (ena * (pos[2] & 0x3F)); // 3 字节

        ena &= pos[0] >> 4;
        bytes += ena;
        cpt = (cpt << (ena * 6)) | (ena * (pos[3] & 0x3F)); // 4 字节

        if ((ena & (pos[0] >> 3))) return ~0L;  // 返回明显错误的码点
        return (cpt | (pos[0] & (0xFF >> (bytes + 1))) << (6 * bytes));
    } // if
    return pos[0];
} // utf8_decode

inline static int32_t utf8_encode(uchar_t ch, char_t buf[4])
{
    int32_t bytes = 0;
    char_t h = 0xF0;

    if (ch < 0x80) {
        bytes = 1;
        buf[0] = ch & 0x7F;
    } else {
        buf[0] = (0x80 | ((ch >>  0) & 0x3F)); bytes += !!((ch >>  0) & 0x3F);
        buf[1] = (0x80 | ((ch >>  6) & 0x3F)); bytes += !!((ch >>  6) & 0x3F);
        buf[2] = (0x80 | ((ch >> 12) & 0x3F)); bytes += !!((ch >> 12) & 0x3F);
        buf[3] = (0x80 | ((ch >> 18) & 0x3F)); bytes += !!((ch >> 18) & 0x3F);

        h <<= (4 - bytes);
        buf[4 - bytes] = h | (buf[4 - bytes] & ~h);

        buf[0] = buf[0] ^ buf[4];
        buf[4] = buf[0] ^ buf[4];
        buf[0] = buf[0] ^ buf[4];

        buf[2] = buf[2] ^ buf[3];
        buf[3] = buf[2] ^ buf[3];
        buf[2] = buf[2] ^ buf[3];
    } // if
    return bytes;
} // utf8_encode

#endif // _AUX_UTF8_H_

