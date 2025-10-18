#include <assert.h>
#include <stdio.h>

#include "str/utf8.h"

uint8_t utf8_bytes_map[128] = {
    // 0x00 ~ 0x7F
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,

    // 0x80 ~ 0xBF
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // 0xC0 ~ 0xDF
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,

    //  0xE0 ~ 0xEF
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,

    // 0xF0 ~ 0xF7
    0x44, 0x44, 0x44, 0x44,

    // 0xF8 ~ 0xFF
    0x00, 0x00, 0x00, 0x00,
};

bool utf8_count(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    const char_t * pos = NULL;
    const char_t * end = NULL;
    uint32_t i = 0;
    uint32_t cnt = 0; // 跟随字节数
    uint32_t chk = 0; // 正确字节数
    uint32_t ena = 0; // 累加开关
    uint32_t max = 0; // 检查字符数上限

    max = *bytes < *chars ? *bytes : *chars; // 字符数 <= 范围内字节数
    end = start + *bytes;
    for (pos = start; i < max && pos < end; ++i, ++pos) {
        if ((ena = (pos[0] >> 7))) {
            cnt += (ena &= (pos[0] >> 6)); chk += (ena & ((pos[1] & 0xC0) == 0x80));
            cnt += (ena &= (pos[0] >> 5)); chk += (ena & ((pos[2] & 0xC0) == 0x80));
            cnt += (ena &= (pos[0] >> 4)); chk += (ena & ((pos[3] & 0xC0) == 0x80));
            cnt &= 0x3 + (ena & (pos[0] >> 3));

            if (cnt == 0) break; // 首字节匹配 0b10xxxxxx 或 0b11111xxx
            if (cnt != chk++) break; // 正确字节少于跟随字节

            pos += cnt;
            cnt = 0;
            chk = 0;
        } // if
    } // for

    pos += chk;
    *bytes = (pos < end ? pos : end) - start; // (start + *bytes) 指向第一个异常字节
    *chars = i;
    return i == max || pos == end;
} // utf8_count

enum {
    VSS_ASCII = 0,
    VSS_TAIL1 = 1,
    VSS_TAIL2 = 2,
    VSS_TAIL3 = 3,
    VSS_ERROR = 4,
};

typedef union UTF8_BYTES {
    uint64_t qword;
    uint8_t  bytes[8];
} utf8_bytes_t, *utf8_bytes_p;

/*
bool utf8_verify_by_lookup(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    static const uint16_t next[5][6] = {
        // leading ones
        // 0          1          2          3          4          5          
        {  VSS_ASCII, VSS_ERROR, VSS_TAIL1, VSS_TAIL2, VSS_TAIL3, VSS_ERROR,  }, // curr = VSS_ASCII
        {  VSS_ERROR, VSS_ASCII, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_TAIL1
        {  VSS_ERROR, VSS_TAIL1, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_TAIL2
        {  VSS_ERROR, VSS_TAIL2, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_TAIL3
        {  VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_ERROR
    };
    uint16_t sts = VSS_ASCII;
    utf8_bytes_t ones = {0};
    utf8_bytes_t ena = {0};
    utf8_bytes_p pos = NULL;
    uint32_t ok_bytes = 0;
    uint32_t ok_chars = 0;
    uint32_t leading_bytes = 0;
    uint32_t tailing_bytes = 0;
    int batches = 0;

    pos = (utf8_bytes_p)((uint64_t)start & ~0x7);
    leading_bytes = (8 - ((void *)pos - (void *)start)) % 8;
    batches = (*bytes - leading_bytes) / 8;
    tailing_bytes = (*bytes - leading_bytes) % 8;
    ena.qword = 0x0101010101010101 << (leading_bytes * 8);

UTF8_VERIFY_AGAIN:
    ones.qword = (pos->qword >> 7) & ena.qword;
    ena.qword &= (pos->qword >> 7);
    ones.qword += (pos->qword >> 6) & ena.qword;
    ena.qword &= (pos->qword >> 6);
    ones.qword += (pos->qword >> 5) & ena.qword;
    ena.qword &= (pos->qword >> 5);
    ones.qword += (pos->qword >> 4) & ena.qword;
    ena.qword &= (pos->qword >> 4);
    ones.qword += (pos->qword >> 3) & ena.qword;

    sts = next[sts][ones.bytes[0]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[1]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[2]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[4]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[5]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[6]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][ones.bytes[7]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;

    if (sts != VSS_ERROR) {
        ++pos;
        ena.qword = 0x0101010101010101;
        if (--batches > 0) goto UTF8_VERIFY_AGAIN;
        if (tailing_bytes > 0) {
            ena.qword >>= (tailing_bytes * 8);
            tailing_bytes = 0;
            goto UTF8_VERIFY_AGAIN;
        } // if
    } // if

    *bytes = ok_bytes;
    *chars = ok_chars;
    return sts == VSS_ASCII;
} // utf8_verify_by_lookup
*/

bool utf8_verify_by_lookup2(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    static const uint16_t next[5][6] = {
        // leading ones
        // 0          1          2          3          4          5          
        {  VSS_ASCII, VSS_ERROR, VSS_TAIL1, VSS_TAIL2, VSS_TAIL3, VSS_ERROR,  }, // curr = VSS_ASCII
        {  VSS_ERROR, VSS_ASCII, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_TAIL1
        {  VSS_ERROR, VSS_TAIL1, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_TAIL2
        {  VSS_ERROR, VSS_TAIL2, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_TAIL3
        {  VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR,  }, // curr = VSS_ERROR
    };
    static const uint8_t tbl[32] = {
        // 00000 ~ 01111
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

        // 10000 ~ 10111
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,

        // 11000 ~ 11011
        0x2, 0x2, 0x2, 0x2,

        // 11100 ~ 11101
        0x3, 0x3,

        // 11110
        0x4,

        // 11111
        0x5,
    };
    uint16_t sts = VSS_ASCII;
    utf8_bytes_t ones = {0};
    utf8_bytes_p pos = NULL;
    uint32_t ok_bytes = 0;
    uint32_t ok_chars = 0;
    uint32_t leading_bytes = 0;
    uint32_t tailing_bytes = 0;
    int batches = 0;

    pos = (utf8_bytes_p)((uint64_t)start & ~0x7);
    leading_bytes = (8 - ((void *)pos - (void *)start)) % 8;
    batches = (*bytes - leading_bytes) / 8;
    tailing_bytes = (*bytes - leading_bytes) % 8;
    ones.qword = pos->qword & (0xFFFFFFFFFFFFFFFF << (leading_bytes * 8));

    if (*bytes == 1) {
        *chars = 1;
        return start[0] <= 0x7F;
    } // if

UTF8_VERIFY_AGAIN:
    sts = next[sts][tbl[ones.bytes[0] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[1] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[2] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[3] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[4] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[5] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[6] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;
    sts = next[sts][tbl[ones.bytes[7] >> 3]]; ok_bytes += sts != VSS_ERROR; ok_chars += sts == VSS_ASCII;

    if (sts != VSS_ERROR) {
        ++pos;
        if (--batches > 0) {
            ones.qword = pos->qword;
            goto UTF8_VERIFY_AGAIN;
        } // if
        if (tailing_bytes > 0) {
            ones.qword = pos->qword & (0xFFFFFFFFFFFFFFFF >> (tailing_bytes * 8));
            tailing_bytes = 0;
            goto UTF8_VERIFY_AGAIN;
        } // if
    } // if

    *bytes = ok_bytes;
    *chars = ok_chars;
    return sts == VSS_ASCII;
} // utf8_verify_by_lookup2

/*
bool utf8_verify_by_addup(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    //uint64_t tbl = 0x007777F700732170;
    //static const uint32_t tbl[2] = { 0x00732170, 0x007777F7 };
    //const uint32_t tbl[2] = { 0x007777F7, 0x00732170 };
    utf8_bytes_t ones = {0};
    utf8_bytes_t ena = {0};
    utf8_bytes_p pos = NULL;
    uint32_t ok_bytes = 0;
    uint32_t ok_chars = 0;
    uint32_t leading_bytes = 0;
    uint32_t tailing_bytes = 0;
    int batches = 0;
    int16_t sum = 0;

    pos = (utf8_bytes_p)((uint64_t)start & ~0x7);
    leading_bytes = (8 - ((void *)pos - (void *)start)) % 8;
    batches = (*bytes - leading_bytes) / 8;
    tailing_bytes = (*bytes - leading_bytes) % 8;
    ena.qword = 0x0101010101010101 << (leading_bytes * 8);

UTF8_VERIFY_AGAIN:
    ones.qword = (pos->qword >> 7) & ena.qword;
    ena.qword &= (pos->qword >> 7);
    ones.qword += (pos->qword >> 6) & ena.qword;
    ena.qword &= (pos->qword >> 6);
    ones.qword += (pos->qword >> 5) & ena.qword;
    ena.qword &= (pos->qword >> 5);
    ones.qword += (pos->qword >> 4) & ena.qword;
    ena.qword &= (pos->qword >> 4);
    ones.qword += (pos->qword >> 3) & ena.qword;

    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[0]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[1]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[2]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[3]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[4]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[5]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[6]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;
    sum += (int16_t)(((0x007777F700732170 >> (sum > 0) * 64) >> ones.bytes[7]) & 0xF); ok_bytes += sum <= 3; ok_chars += sum == 0;

    if (sum <= 3) {
        ++pos;
        ena.qword = 0x0101010101010101;
        if (--batches > 0) goto UTF8_VERIFY_AGAIN;
        if (tailing_bytes > 0) {
            ena.qword >>= (tailing_bytes * 8);
            tailing_bytes = 0;
            goto UTF8_VERIFY_AGAIN;
        } // if
    } // if

    *bytes = ok_bytes;
    *chars = ok_chars;
    return sum == 0;
} // utf8_verify_by_addup
*/
