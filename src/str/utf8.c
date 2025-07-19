#include "str/utf8.h"

void * utf8_check(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes)
{
    char_t follower = 0xC0;
    uint32_t ret_bytes = 0;
    uint32_t cnt = 0;
    uint32_t bound = 0;
    void * loc = NULL;
    char_t * pos = NULL;

    bound = index;
    pos = (char_t *)begin;

UTF8_CHECK_AGAIN:
    while (cnt < bound && pos < end) {
        ret_bytes = measure_utf8(pos);
        switch (ret_bytes) {
            case 1: break;
            case 4: follower |= pos[3] & 0xC0;
            case 3: follower |= pos[2] & 0xC0;
            case 2: follower |= pos[1] & 0xC0;
                if (follower == 0xC0) break;
            default:
                goto UTF8_CHECK_ERROR; // 存在异常字节，返回空串。
        } // switch
        cnt += 1;
        pos += ret_bytes;
    } // while

    if (! loc) {
        loc = pos;
        cnt = 0;
        bound = *chars;
        goto UTF8_CHECK_AGAIN;
    } // if
    
    *chars = cnt;
    *bytes = pos - loc;
    return loc;

UTF8_CHECK_ERROR:
    *chars = 0;
    *bytes = 0;
    return NULL;
} // utf8_check
