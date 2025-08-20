#include <criterion/criterion.h>

#ifndef UTF8_SOURCE
#define UTF8_SOURCE 1
#include "str/utf8.c"
#endif

const char_t s1[] = {"A"};
const char_t s2[] = {"\xDA\xBF"};
const char_t s3[] = {"\xEA\xBF\x80"};
const char_t s4[] = {"\xF2\xBF\x80\xA5"};

const char_t b1_1[] = {"\x80"};
const char_t b1_2[] = {"\xA0"};
const char_t b2[] = {"\xDA\xFF"};
const char_t b3[] = {"\xEA\xBF\xC0"};
const char_t b4[] = {"\xF2\xBF\x80\x25"};

Test(Function, utf8_measure)
{
    int32_t ret = 0;

    ret = utf8_measure(s1);
    cr_expect(ret == 1, "utf8_measure('A'): expect %d, got %d", 1, ret);

    ret = utf8_measure(s2);
    cr_expect(ret == 2, "utf8_measure('\\xDA\\xBF'): expect %d, got %d", 2, ret);

    ret = utf8_measure(s3);
    cr_expect(ret == 3, "utf8_measure('\\xEA\\xBF\\x80'): expect %d, got %d", 3, ret);

    ret = utf8_measure(s4);
    cr_expect(ret == 4, "utf8_measure('\\xFA\\xBF\\x80\\x7D'): expect %d, got %d", 4, ret);

    ret = utf8_measure(b1_1);
    cr_expect(ret == 0, "utf8_measure('\\x80'): expect %d, got %d", 0, ret);

    ret = utf8_measure(b1_2);
    cr_expect(ret == 0, "utf8_measure('\\xA0'): expect %d, got %d", 0, ret);
} // utf_measure

Test(Function, utf8_count)
{
    int32_t bytes = 0;
    int32_t chars = 0;

    bytes = utf8_count(s1, sizeof(s1) - 1, &chars);
    cr_expect(bytes == 1, "utf8_measure('A') return incorrect bytes: expect %d, got %d", 1, bytes);
    cr_expect(chars == 1, "utf8_measure('A') return incorrect chars: expect %d, got %d", 1, chars);
} // utf8_count
