#include "str/utf8.h"

int32_t utf8_count(const char_t * start, int32_t size, int32_t * chars)
{
    uint8_t * pos = NULL;
    uint8_t follower = 0xC0;
    int32_t i = 0;
    int32_t max = 0;
    int32_t r_bytes = 0;
    int32_t ret = -1;

    max = (chars && *chars <= size) ? *chars : size; // UTF-8 字符数必然少于或等于字节数
    for (pos = (uint8_t *)start; i < max && pos < start + size; ++i, pos += r_bytes) {
        r_bytes = utf8_measure(pos);
        switch (r_bytes) {
            case 1: break;
            case 4: follower |= pos[3] & 0xC0;
            case 3: follower |= pos[2] & 0xC0;
            case 2: follower |= pos[1] & 0xC0;
                if (follower == 0xC0) break;
            default:
                goto UTF8_LOCATE_ERROR; // 存在异常字节
        } // switch
    } // for

    ret = pos - start;

UTF8_LOCATE_ERROR:
    if (chars) *chars = i;
    return ret;
} // utf_count
