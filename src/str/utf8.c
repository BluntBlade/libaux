#include <assert.h>
#include <stdio.h>

#include "str/utf8.h"
#include "str/misc.h"

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

bool utf8_verify_plain(const char_t * start, uint32_t * bytes)
{
    int i = 0;
    while (i < *bytes) {
        if (start[i] <= 0x7F) {
            i += 1;
            continue;
        } // if

        if ((start[i] >> 6) == 0x02 || (start[i] >> 3) == 0x1F) {
            *bytes = i;
            return false;
        } // if

        if ((start[i] >> 5) == 0x06) {
            if ((start[i + 1] >> 6) != 0x02) {
                *bytes = i + 1;
                return false;
            } // if
            i += 2;
            continue;
        } // if

        if ((start[i] >> 4) == 0x0E) {
            if ((start[i + 1] >> 6) != 0x02) {
                *bytes = i + 1;
                return false;
            } // if
            if ((start[i + 2] >> 6) != 0x02) {
                *bytes = i + 2;
                return false;
            } // if
            i += 3;
            continue;
        } // if

        if ((start[i] >> 3) == 0x1E) {
            if ((start[i + 1] >> 6) != 0x02) {
                *bytes = i + 1;
                return false;
            } // if
            if ((start[i + 2] >> 6) != 0x02) {
                *bytes = i + 2;
                return false;
            } // if
            if ((start[i + 3] >> 6) != 0x02) {
                *bytes = i + 3;
                return false;
            } // if
            i += 4;
            continue;
        } // if
    } // while

    return i == *bytes;
} // utf8_verify_plain

enum {
    VSS_ASCII = 0,
    VSS_TAIL1 = 1,
    VSS_TAIL2 = 2,
    VSS_TAIL3 = 3,
    VSS_ERROR = 4,
};

typedef union UTF8_CHUNK {
    uint64_t qword;
    uint8_t  bytes[8];
} utf8_chunk_t, *utf8_chunk_p;

bool utf8_verify_by_lookup(const char_t * start, uint32_t * bytes)
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

    utf8_chunk_t ones = {0};
    utf8_chunk_p pos = NULL;
    uint32_t ok_bytes = 0;
    uint32_t ng_bytes = 0;
    uint32_t leads = 0;
    uint32_t tails = 0;
    uint32_t chunks = 0;
    uint32_t chunk_size = sizeof(utf8_chunk_t);
    uint16_t curr_sts = VSS_ASCII;
    uint16_t prev_sts = VSS_ASCII;
    bool last = false;

    if (*bytes == 0) return true;
    if (*bytes == 1) return (*bytes = (start[0] <= 0x7F));

    str_span(start, *bytes, chunk_size, &leads, &chunks, &tails);

    pos = (utf8_chunk_p)str_round_up((uint64_t)start, chunk_size);
    last = pos != (utf8_chunk_p)str_round_up((uint64_t)start + *bytes, chunk_size);

    pos -= (void *)pos != (void *)start;
    chunks -= (leads == 0 && chunks > 0); // 没有前导字节且块数大于 0 ，则必须少循环 1 次

    ones.qword = pos->qword & (0xFFFFFFFFFFFFFFFF << (leads * chunk_size));

UTF8_VERIFY_AGAIN:
    ok_bytes += chunk_size;

    curr_sts = next[curr_sts][tbl[ones.bytes[0] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[1] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[2] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[3] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[4] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[5] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[6] >> 3]];
    curr_sts = next[curr_sts][tbl[ones.bytes[7] >> 3]];

    if (curr_sts != VSS_ERROR) {
        prev_sts = curr_sts;
        pos += 1;

        if (chunks > 0) {
            chunks -= 1;
            ones.qword = pos->qword;
            goto UTF8_VERIFY_AGAIN;
        } // if
        if (last) {
            last = false;
            ones.qword = pos->qword & (0xFFFFFFFFFFFFFFFF >> (tails * chunk_size));
            goto UTF8_VERIFY_AGAIN;
        } // if
    } else {
        curr_sts = prev_sts;
        curr_sts = next[curr_sts][tbl[ones.bytes[0] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[1] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[2] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[3] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[4] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[5] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[6] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
        curr_sts = next[curr_sts][tbl[ones.bytes[7] >> 3]]; ng_bytes += curr_sts == VSS_ERROR;
    } // if

    *bytes = ok_bytes - leads - (curr_sts == VSS_ASCII ? tails : ng_bytes);
    return curr_sts == VSS_ASCII;
} // utf8_verify_by_lookup
