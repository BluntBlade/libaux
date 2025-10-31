#include <assert.h>
#include <stdio.h>

#include "str/utf8.h"
#include "str/misc.h"

enum {
    TKN_NUL = 0,
    TKN_ASCII = 1,
    TKN_HEAD2 = 2,
    TKN_HEAD3 = 3,
    TKN_HEAD4 = 4,
    TKN_TAIL1 = 5,
    TKN_ERROR = 6,
};

inline static uint8_t get_token(const char_t ch)
{
    uint8_t tkns[128] = {
        // 0x00 ~ 0x7F
        0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
        0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,

        // 0x80 ~ 0xBF
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,

        // 0xC0 ~ 0xDF
        0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,

        //  0xE0 ~ 0xEF
        0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,

        // 0xF0 ~ 0xF7
        0x44, 0x44, 0x44, 0x44,

        // 0xF8 ~ 0xFF
        0x66, 0x66, 0x66, 0x66,
    };
    return (tkns[ch >> 1] >> ((ch & 0x1) << 2)) & 0xF;
} // get_token

uint32_t utf8_measure_by_lookup(const char_t * pos)
{
    static uint8_t chars[8] = {0, 1, 2, 3, 4, 0, 0, 0};
    return chars[get_token(*pos)];
} // utf8_measure_by_lookup

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

bool utf8_verify_plain(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    int i = 0;
    *chars = 0;
    while (i < *bytes) {
        if (start[i] <= 0x7F) {
            i += 1;
            *chars += 1;
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
            *chars += 1;
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
            *chars += 1;
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
            *chars += 1;
            continue;
        } // if
    } // while

    return i == *bytes;
} // utf8_verify_plain

enum {
    VSS_END = 0,
    VSS_ASCII = 1,
    VSS_TAIL1 = 2,
    VSS_TAIL2 = 3,
    VSS_TAIL3 = 4,
    VSS_ERROR = 5,
};

inline static uint8_t move_next(const uint8_t sts, const char_t ch)
{
    static const uint8_t next[6][7] = {
        // input token
        // TKN_NUL    TKN_ASCII  TKN_HEAD2  TKN_HEAD3  TKN_HEAD4  TKN_TAIL1  TKN_ERROR
        {  VSS_END,   VSS_END,   VSS_END,   VSS_END,   VSS_END,   VSS_END,   VSS_END,   }, // curr = VSS_END
        {  VSS_END,   VSS_ASCII, VSS_TAIL1, VSS_TAIL2, VSS_TAIL3, VSS_ERROR, VSS_ERROR, }, // curr = VSS_ASCII
        {  VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ASCII, VSS_ERROR, }, // curr = VSS_TAIL1
        {  VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_TAIL1, VSS_ERROR, }, // curr = VSS_TAIL2
        {  VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_TAIL2, VSS_ERROR, }, // curr = VSS_TAIL3
        {  VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, VSS_ERROR, }, // curr = VSS_ERROR
    };
    return next[sts][get_token(ch)];
} // move_next

uint8_t verify_part(uint8_t sts, const char_t * pos, const uint32_t bytes, uint32_t * ng_bytes, uint32_t * chars)
{
    switch(bytes) {
        case 8: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 7: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 6: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 5: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 4: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 3: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 2: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        case 1: sts = move_next(sts, *pos++); *ng_bytes += sts == VSS_ERROR; *chars += sts == VSS_ASCII;
        default: break;
    } // switch
    return sts;
} // verify_part

bool utf8_verify_by_lookup(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    const char_t * pos = NULL;
    const char_t * begin = NULL;
    const char_t * end = NULL;
    uint32_t ok_bytes = 0;
    uint32_t ng_bytes = 0;
    uint32_t leads = 0;
    uint32_t tails = 0;
    uint32_t chunks = 0;
    uint8_t curr_sts = VSS_ASCII;
    uint8_t prev_sts = VSS_ASCII;
    const uint32_t chunk_size = 8;

    *chars = 0;
    pos = start;
    if (*bytes <= chunk_size) {
        curr_sts = verify_part(curr_sts, pos, *bytes, &ng_bytes, chars);
        *bytes -= ng_bytes;
        return curr_sts == VSS_ASCII;
    } // if

    str_span(start, *bytes, chunk_size, &begin, &leads, &chunks, &tails, &end);

    if (leads > 0) {
        ok_bytes = chunk_size - leads;
        curr_sts = verify_part(curr_sts, pos, ok_bytes, &ng_bytes, chars);
        if (curr_sts == VSS_ERROR) goto UTF8_VERIFY_BY_LOOKUP_END;
        pos += ok_bytes;
    } // if

    while (chunks > 0) {
        prev_sts = curr_sts;
        curr_sts = move_next(curr_sts, pos[0]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[1]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[2]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[3]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[4]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[5]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[6]); *chars += curr_sts == VSS_ASCII;
        curr_sts = move_next(curr_sts, pos[7]); *chars += curr_sts == VSS_ASCII;

        ok_bytes += chunk_size;
        if (curr_sts == VSS_ERROR) {
            curr_sts = verify_part(prev_sts, pos, chunk_size, &ng_bytes, chars);
            goto UTF8_VERIFY_BY_LOOKUP_END;
        } // if

        pos += chunk_size;
        chunks -= 1;
    } // while

    if (tails > 0) {
        ok_bytes += chunk_size - tails;
        curr_sts = verify_part(curr_sts, pos, chunk_size - tails, &ng_bytes, chars);
    } // if

UTF8_VERIFY_BY_LOOKUP_END:
    *bytes = ok_bytes - ng_bytes;
    return curr_sts == VSS_ASCII;
} // utf8_verify_by_lookup
