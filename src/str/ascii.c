#include "types.h"
#include "str/ascii.h"

int32_t ascii_count(void * start, int32_t size, int32_t * chars)
{
    void * pos = NULL;
    int32_t i = 0;
    int32_t max = 0;

    max = (chars && *chars < size) ? *chars : size;
    for (pos = start; i < max; ++i, ++pos) {
        if (measure_ascii(pos) == 0) return -1;
    } // for

    if (chars) *chars = max;
    return max;
} // ascii_count
