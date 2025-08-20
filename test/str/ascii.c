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

Test(Function, ascii_count)
{
    const char_t s1[] = {"A"};
    const char_t s2[] = {"AB"};
    const char_t s3[] = {"\x20\x40\x7F"};

    const char_t b1[] = {"\0"};
    const char_t b2[] = {"A\0"};
    const char_t b3[] = {"\0AB"};

    int32_t size = 0;
    int32_t bytes = 0;
    int32_t chars = 0;

    size = sizeof(s1) - 1;
    bytes = ascii_count(s1, size, &chars);
    cr_expect(bytes == size, "ascii_count(s1) return incorrect bytes: expect %d, got %d", size, bytes);
    cr_expect(chars == size, "ascii_count(s1) return incorrect chars: expect %d, got %d", size, chars);

    chars = size = sizeof(s2) - 1;
    bytes = ascii_count(s2, size, &chars);
    cr_expect(bytes == size, "ascii_count(s2) return incorrect bytes: expect %d, got %d", size, bytes);
    cr_expect(chars == size, "ascii_count(s2) return incorrect chars: expect %d, got %d", size, chars);

    chars = sizeof(s3);
    size = sizeof(s3) - 1;
    bytes = ascii_count(s3, size, &chars);
    cr_expect(bytes == size, "ascii_count(s3) return incorrect bytes: expect %d, got %d", size, bytes);
    cr_expect(chars == size, "ascii_count(s3) return incorrect chars: expect %d, got %d", size, chars);

    chars = size = sizeof(b1) - 1;
    bytes = ascii_count(b1, size, &chars);
    cr_expect(bytes == -1, "ascii_count(b1) return incorrect bytes: expect %d, got %d", -1, bytes);
    cr_expect(chars == size, "ascii_count(b1) return incorrect chars: expect %d, got %d", size, chars);

    chars = size = sizeof(b2) - 1;
    bytes = ascii_count(b2, size, &chars);
    cr_expect(bytes == -1, "ascii_count(b2) return incorrect bytes: expect %d, got %d", -1, bytes);
    cr_expect(chars == size, "ascii_count(b2) return incorrect chars: expect %d, got %d", size, chars);

    chars = size = sizeof(b3) - 1;
    bytes = ascii_count(b3, size, &chars);
    cr_expect(bytes == -1, "ascii_count(b3) return incorrect bytes: expect %d, got %d", -1, bytes);
    cr_expect(chars == size, "ascii_count(b3) return incorrect chars: expect %d, got %d", size, chars);
} // ascii_count
