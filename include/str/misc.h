#ifndef _AUX_STR_MISC_H_
#define _AUX_STR_MISC_H_ 1

#ifdef AUX_TESTING
#include <assert.h>
#endif

#include "types.h"

inline static uint64_t str_round_up(uint64_t integer, uint64_t alignment)
{
    return (integer + (alignment - 1)) & ~(alignment - 1);
} // str_round_up

// 功能：计算字节范围跨越定长块的数量及前导和后随填充字节数
// 参数：
//     start        IN      字节范围起始地址
//     bytes        IN      字节范围长度
//     alignment    IN      对齐长度（块长度）
//     leads        OUT     前导填充字节数
//     chunks       OUT     定长块数
//     tails        OUT     后随填充字节数
// 返回值：
//     无
// 说明：
//     +--------+--------+--------+--------+
//     |XXXXXXXX|LLLBBBBB|BBBTTTTT|XXXXXXXX|
//     +--------+--------+--------+--------+
//     X = 未知字节
//     B = 有效字节（范围内）
//     L = 前导字节
//     T = 后随字节
inline static void str_span(const char_t * start, const uint32_t bytes, const uint32_t alignment, const char_t ** begin, uint32_t * leads, uint32_t * chunks, uint32_t * tails, const char_t ** end)
{
    //    +--> begin
    //   /    +--> start
    //  /    /   +--> end
    // /    /   /
    // v---v----v--------+
    // |   BBB  |        |
    // +--------+--------+

    *begin = (const char_t *)str_round_up((uint64_t)start, alignment);
    *end = (const char_t *)str_round_up((uint64_t)start + bytes, alignment);
    *chunks = *end - *begin;

    *begin = *begin == start ? *begin : *begin - alignment; 

#ifdef AUX_TESTING
    assert(*end - *begin > bytes);
#endif

    *leads = start - *begin;
    *tails = (*end - *begin) - *leads - bytes;
    *chunks = (*chunks == 0) ? *chunks : (*end - *begin - str_round_up(*leads, alignment) - str_round_up(*tails, alignment)) / alignment;
} // str_span

#endif // _AUX_STR_MISC_H_
