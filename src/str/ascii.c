#include "types.h"
#include "str/ascii.h"

void * ascii_check(void * begin, void * end, uint32_t index, uint32_t * chars)
{
    uint32_t cnt = 0;
    uint32_t bound = index;
    void * loc = NULL;
    char_t * pos = (char_t *)begin;

ASCII_LOCATE_AGAIN:
    while (cnt < bound && pos < end) {
        if (measure_ascii(pos) == 0) goto ASCII_LOCATE_ERROR; // 提前终止，返回空串。
        cnt += 1;
        pos += 1;
    } // while

    if (! loc) {
        loc = pos;
        cnt = 0;
        bount = *chars;
        goto ASCII_LOCATE_AGAIN;
    } // if
    
    *chars = cnt;
    return loc;

ASCII_LOCATE_ERROR:
    *chars = 0;
    return NULL;
} // ascii_check
