#ifndef _AUX_UTF8_H_
#define _AUX_UTF8_H_ 1

// 引用: https://en.wikipedia.org/wiki/UTF-8
//
// UTF-8 encodes code points in one to four bytes, depending on the value of the code point.
// In the following table, the characters u to z are replaced by the bits of the code point, from the positions U+uvwxyz:
//
// Code point ↔  UTF-8 conversion
//
// +------------------+-----------------+------------+------------+------------+------------+
// | First Code Point | Last Code Point | Byte 1     | Byte 2     | Byte 3     | Byte 4     |
// |------------------+-----------------+------------+------------+------------+------------+
// | U+0000           | U+007F          | 0yyyzzzz   |            |            |            |
// | U+0080           | U+07FF          | 110xxxyy   | 10yyzzzz   |            |            |
// | U+0800           | U+FFFF          | 1110wwww   | 10xxxxyy   | 10yyzzzz   |            |
// | U+010000         | U+10FFFF        | 11110uvv   | 10vvwwww   | 10xxxxyy   | 10yyzzzz   |
// +------------------+-----------------+------------+------------+------------+------------+

#include "types.h"

extern uint8_t utf8_bytes_map[128];

// 功能：测量单个 UTF-8 字符包含的字节数
// 参数：
//     pos      IN  字符串指针，不能为 NULL
// 返回值：
//     0 <          字节数
//     0            存在异常字节
// 说明：
//     逐个检查字节，直观但较慢的逻辑。
inline static uint32_t utf8_measure_plain(const char_t * pos)
{
    char_t ch = pos[0];
    if ((ch & 0x80) == 0) return 1;
    if (((ch <<= 1) & 0x80) == 0) return 0;
    if (((ch <<= 1) & 0x80) == 0) return 2;
    if (((ch <<= 1) & 0x80) == 0) return 3;
    if (((ch <<= 1) & 0x80) == 0) return 4;
    return 0;
} // utf8_measure_plain

// 功能：测量单个 UTF-8 字符包含的字节数（查表法）
// 参数：
//     pos      IN  字符串指针，不能为 NULL
// 返回值：
//     0 <          字节数
//     0            存在异常字节
// 说明：
//     逻辑最简单，需加载表格到缓存中，会影响性能。
inline static uint32_t utf8_measure_by_lookup(const char_t * pos)
{
    return (utf8_bytes_map[pos[0] / 2] >> ((pos[0] & 0x1) * 4)) & 0x7;
} // utf8_measure_by_lookup

// 功能：测量单个 UTF-8 字符包含的字节数（累加法）
// 参数：
//     pos      IN  字符串指针，不能为 NULL
// 返回值：
//     0 <          字节数
//     0            存在异常字节
// 说明：
//     平衡机器码字节数、缓存命中率和计算性能的逻辑实现。
inline static uint32_t utf8_measure_by_addup(const char_t * pos)
{
    uint32_t ena = 0;    // 累加开关
    uint32_t bytes = 1;  // 字节数

    bytes -= (ena  = (pos[0] >> 7));            // 首字节可能以 0b10 打头，预先减 1
    bytes += (ena &= (pos[0] >> 6)) * 2;        // 2 字节
    bytes += (ena &= (pos[0] >> 5));            // 3 字节
    bytes += (ena &= (pos[0] >> 4));            // 4 字节
    return bytes * !(ena & (pos[0] >> 3));      // 首字节是 0b11111xxx ，返回 0
} // utf8_measure_by_addup

#define utf8_measure utf8_measure_by_addup

// 功能：计算给定范围包含多少个 UTF-8 字符
// 参数：
//     start    IN  起始地址，不能为 NULL
//     bytes    IN  范围长度（字节数）
//     chars    IO  入参：最大字符数，不能为 NULL
//                  出参：包含字符数
// 返回值：
//     true         编码正确
//     false        编码错误
bool utf8_count(const char_t * start, uint32_t * bytes, uint32_t * chars);

// 功能：解码 UTF-8 字符
// 参数：
//     pos      IN  起始地址，不能为 NULL
//     ch       OUT 码点值指针，不能为 NULL
// 返回值：
//     0 <=         解码字节数
//     < 0          存在异常字节
inline static int32_t utf8_decode(const char_t * pos, uchar_t * ch)
{
    uchar_t code = 0;   // Unicode 码点
    int32_t bad = 0;    // 异常字节数
    int32_t ena = 1;    // 操作开关
    int32_t bytes = 1;  // 字节数

    if (pos[0] <= 0x7F) {
        *ch = pos[0];
        return 1;
    } // if

    if ((pos[0] >> 6) == 0x2) return -1; // 异常字节

    code = (pos[1] & 0x3F); // 2 字节
    bad = (pos[1] & 0xC0) != 0x80;

    ena &= pos[0] >> 5;
    code = (code << (ena * 6)) | (ena * (pos[2] & 0x3F));
    bad += ena * ((pos[2] & 0xC0) != 0x80);
    bytes += ena; // 3 字节

    ena &= pos[0] >> 4;
    code = (code << (ena * 6)) | (ena * (pos[3] & 0x3F));
    bad += ena * ((pos[3] & 0xC0) != 0x80);
    bytes += ena; // 4 字节

    if (bad > 0 || (ena & (pos[0] >> 3))) return -1; // 异常字节
    *ch = code | ((pos[0] & (0x3F >> bytes)) << (6 * bytes));
    return bytes + 1;
} // utf8_decode

// 功能：编码 UTF-8 字符
// 参数：
//     ch       IN  Unicode 码点值
//     seq      OUT 编码结果
// 返回值：
//     1 <=         编码字节数
inline static int32_t utf8_encode(uchar_t ch, char_t seq[4])
{
    int32_t ena = 0;
    int32_t bytes = 0;
    char_t h = 0xF0; // 0b11110000

    if (ch < 0x80) {
        seq[0] = ch & 0x7F;
        return 1;
    } // if

    ena |= !!(ch >> 16); seq[bytes] = ena * (0x80 | ((ch >> 18) & 0x3F)); bytes += ena;
    ena |= !!(ch >> 11); seq[bytes] = ena * (0x80 | ((ch >> 12) & 0x3F)); bytes += ena;
    seq[bytes++] = (0x80 | ((ch >> 6) & 0x3F));
    seq[bytes++] = (0x80 | ((ch >> 0) & 0x3F));

    h <<= (4 - bytes);
    seq[0] = h | (seq[0] & ~h);
    return bytes;
} // utf8_encode

#endif // _AUX_UTF8_H_

