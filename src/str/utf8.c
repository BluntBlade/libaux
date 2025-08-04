#include "str/utf8.h"

void * utf8_check(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes)
{
    char_t follower = 0xC0;
    uint32_t ret_bytes = 0;
    uint32_t rmd = 0;
    void * loc = NULL;
    char_t * pos = NULL;

    rmd = index + 1;
    pos = (char_t *)begin;

UTF8_CHECK_AGAIN:
    while (--rmd > 0 && pos < end) {
        ret_bytes = measure_utf8(pos);
        switch (ret_bytes) {
            case 1: break;
            case 4: follower |= pos[3] & 0xC0;
            case 3: follower |= pos[2] & 0xC0;
            case 2: follower |= pos[1] & 0xC0;
                if (follower == 0xC0) break;
            default:
                // 存在异常字节，返回空串。
                *chars = 0;
                *bytes = 0;
                return NULL;
        } // switch
        pos += ret_bytes;
    } // while

    if (! loc) {
        loc = pos;
        rmd = *chars + 1;
        goto UTF8_CHECK_AGAIN;
    } // if
    
    *chars -= rmd;
    *bytes = pos - loc;
    return loc;
} // utf8_check

int32_t utf8_count(void * start, int32_t size, int32_t * chars)
{
    uint8_t * pos = NULL;
    uint8_t follower = 0xC0;
    int32_t i = 0;
    int32_t max = 0;
    int32_t r_bytes = 0;
    int32_t ret = -1;

    max = (chars && *chars <= size) ? *chars : size; // UTF-8 字符数必然少于或等于字节数
    for (pos = (uint8_t *)start; i < max && pos < start + size; ++i, pos += r_bytes) {
        r_bytes = measure_utf8(pos);
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
