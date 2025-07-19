#include "str/utf8.h"

void * utf8_locate(void * begin, void * end, uint32_t index, uint32_t * chars)
{
    char_t follower = 0xC0;
    uint32_t bytes = 0;
    uint32_t cnt = 0;
    uint32_t bound = index;
    void * loc = NULL;
    char_t * pos = (char_t *)begin;

UTF8_LOCATE_AGAIN:
    while (cnt < bound && pos < end) {
        bytes = measure_utf8(pos);
        switch (bytes) {
            case 1: break;
            case 4: follower |= pos[3] & 0xC0;
            case 3: follower |= pos[2] & 0xC0;
            case 2: follower |= pos[1] & 0xC0;
                if (follower == 0xC0) break;
            default:
                goto UTF8_LOCATE_ERROR; // 存在异常字节，返回空串。
        } // switch
        cnt += 1;
        pos += bytes;
    } // while

    if (! loc) {
        loc = pos;
        cnt = 0;
        bount = *chars;
        goto UTF8_LOCATE_AGAIN;
    } // if
    
    *chars = cnt;
    return loc;

UTF8_LOCATE_ERROR:
    *chars = 0;
    return NULL;
} // utf8_locate
