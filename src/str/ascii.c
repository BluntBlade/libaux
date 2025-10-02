#include <assert.h>
#include <stdio.h>

#include "str/ascii.h"

bool ascii_count(const char_t * start, uint32_t * bytes, uint32_t * chars)
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
} // ascii_count
