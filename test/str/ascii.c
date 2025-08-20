#include <criterion/criterion.h>

#ifndef ASCII_SOURCE
#define ASCII_SOURCE 1
#include "str/ascii.c"
#endif

Test(Function, ascii_measure)
{
    char_t ch = 0;
    cr_expect(ascii_measure(&ch) == 0, "ascii_measure('NUL') return 1");

    ch = 'A';
    cr_expect(ascii_measure(&ch) == 1, "ascii_measure('A') return 0");

    ch = '\x01';
    cr_expect(ascii_measure(&ch) == 1, "ascii_measure('\\x01') return 0");

    ch = '\x7F';
    cr_expect(ascii_measure(&ch) == 1, "ascii_measure('\\x7F') return 0");

    ch = '\x80';
    cr_expect(ascii_measure(&ch) == 0, "ascii_measure('\\x80') return 1");
} // ascii_measure
