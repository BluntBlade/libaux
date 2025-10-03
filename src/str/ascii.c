#include <assert.h>
#include <stdio.h>

#include "str/ascii.h"

bool ascii_count_plain(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    const char_t * pos = NULL;
    uint32_t i = 0;
    uint32_t max = 0;

    assert(bytes != NULL);
    assert(chars != NULL);

    max = *bytes < *chars ? *bytes : *chars;
    for (pos = start; i < max && ascii_measure(pos) > 0; ++i, ++pos) ;

    *bytes = i;
    *chars = i;
    return i == max;
} // ascii_count_plain

bool ascii_count_unroll(const char_t * start, uint32_t * bytes, uint32_t * chars)
{
    const char_t * pos = start;
    uint32_t ena = 1;
    uint32_t i = 0;
    uint32_t cnt = 0;
    uint32_t max = 0;

    assert(bytes != NULL);
    assert(chars != NULL);

    max = *bytes < *chars ? *bytes : *chars;
    pos = start;
    i = max / 4;
    switch (max % 4) {
        do {
                cnt += (ena &= ascii_measure(pos++));
        case 3: cnt += (ena &= ascii_measure(pos++));
        case 2: cnt += (ena &= ascii_measure(pos++));
        case 1: cnt += (ena &= ascii_measure(pos++));
        } while ((ena * i--) > 0);
        default: break;
    } // switch

    *bytes = cnt;
    *chars = cnt;
    return cnt == max;
} // ascii_count_unroll
