#include "str/utf8.h"

int32_t utf8_count(const char_t * start, int32_t size, int32_t * chars)
{
    const char_t * pos = NULL;
    uint8_t follower = 0xC0;
    int32_t i = 0;
    int32_t max = 0;
    int32_t r_bytes = 0;

    max = (chars && 0 < *chars && *chars < size) ? *chars : size; // UTF-8 字符数必然少于或等于字节数
    for (pos = start; i < max && pos < start + size; ++i, pos += r_bytes) {
        r_bytes = utf8_measure(pos);
        switch (r_bytes) {
            case 1: break;
            case 4: follower |= pos[3] & 0xC0;
            case 3: follower |= pos[2] & 0xC0;
            case 2: follower |= pos[1] & 0xC0;
                if (follower == 0xC0) break;
            default:
                return -1; // 存在异常字节
        } // switch
    } // for

    if (chars) *chars = i;
    return pos - start;
} // utf_count
