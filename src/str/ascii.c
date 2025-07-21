#include "types.h"
#include "str/ascii.h"

void * ascii_check(void * begin, void * end, uint32_t index, uint32_t * chars, uint32_t * bytes)
{
    void * loc = NULL;
    char_t * pos = (char_t *)begin;
    uint32_t rmd = index + 1;

ASCII_CHECK_AGAIN:
    while (--rmd > 0 && pos < end) {
        if (measure_ascii(pos) == 0) {
            // 提前终止，返回空串。
            *chars = 0;
            *bytes = 0;
            return NULL;
        } // if
        pos += 1;
    } // while

    if (! loc) {
        loc = pos;
        rmd = *chars + 1;
        goto ASCII_CHECK_AGAIN;
    } // if
    
    *chars -= rmd;
    *bytes = pos - loc;
    return loc;
} // ascii_check
