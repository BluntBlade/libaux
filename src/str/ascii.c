#include "str/ascii.h"

int32_t ascii_count(const char_t * start, int32_t size, int32_t * chars)
{
    const char_t * pos = NULL;
    int32_t i = 0;
    int32_t max = 0;

    max = (chars && 0 < *chars && *chars < size) ? *chars : size;
    for (pos = start; i < max; ++i, ++pos) {
        if (ascii_measure(pos) == 0) return -1;
    } // for

    if (chars) *chars = max;
    return max;
} // ascii_count
