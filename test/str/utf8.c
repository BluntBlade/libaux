#include <criterion/criterion.h>

#ifndef UTF8_SOURCE
#define UTF8_SOURCE 1
#include "str/utf8.c"
#endif

typedef struct TEST_DATA {
    int32_t bytes;
    int32_t chars;
    const char_t name[80];
    const char_t str[80];
    const char_t repr[80];
} test_data_t;

#define S1_STR "A"
#define S1_REPR "A"
#define S2_STR "\xDA\xBF"
#define S2_REPR "\\xDA\\xBF"
#define S3_STR "\xEA\xBF\x80"
#define S3_REPR "\\xEA\\xBF\\x80"
#define S4_STR "\xF2\xBF\x80\xA5"
#define S4_REPR "\\xF2\\xBF\\x80\\xA5"

test_data_t s1 = {.bytes = 1, .chars = 1, .name = {"s1"}, .str = {S1_STR}, .repr = {S1_REPR}};
test_data_t s2 = {.bytes = 2, .chars = 1, .name = {"s2"}, .str = {S2_STR}, .repr = {S2_REPR}};
test_data_t s3 = {.bytes = 3, .chars = 1, .name = {"s3"}, .str = {S3_STR}, .repr = {S3_REPR}};
test_data_t s4 = {.bytes = 4, .chars = 1, .name = {"s4"}, .str = {S4_STR}, .repr = {S4_REPR}};

test_data_t sc[] = {
    {.bytes = 2, .chars = 2, .name = {"s11"}, .str = {S1_STR S1_STR}, .repr = {S1_REPR S1_REPR}},
    {.bytes = 3, .chars = 2, .name = {"s12"}, .str = {S1_STR S2_STR}, .repr = {S1_REPR S2_REPR}},
    {.bytes = 4, .chars = 2, .name = {"s13"}, .str = {S1_STR S3_STR}, .repr = {S1_REPR S3_REPR}},
    {.bytes = 5, .chars = 2, .name = {"s14"}, .str = {S1_STR S4_STR}, .repr = {S1_REPR S4_REPR}},

    {.bytes = 3, .chars = 2, .name = {"s21"}, .str = {S2_STR S1_STR}, .repr = {S2_REPR S1_REPR}},
    {.bytes = 4, .chars = 2, .name = {"s22"}, .str = {S2_STR S2_STR}, .repr = {S2_REPR S2_REPR}},
    {.bytes = 5, .chars = 2, .name = {"s23"}, .str = {S2_STR S3_STR}, .repr = {S2_REPR S3_REPR}},
    {.bytes = 6, .chars = 2, .name = {"s24"}, .str = {S2_STR S4_STR}, .repr = {S2_REPR S4_REPR}},

    {.bytes = 4, .chars = 2, .name = {"s31"}, .str = {S3_STR S1_STR}, .repr = {S3_REPR S1_REPR}},
    {.bytes = 5, .chars = 2, .name = {"s32"}, .str = {S3_STR S2_STR}, .repr = {S3_REPR S2_REPR}},
    {.bytes = 6, .chars = 2, .name = {"s33"}, .str = {S3_STR S3_STR}, .repr = {S3_REPR S3_REPR}},
    {.bytes = 7, .chars = 2, .name = {"s34"}, .str = {S3_STR S4_STR}, .repr = {S3_REPR S4_REPR}},

    {.bytes = 5, .chars = 2, .name = {"s41"}, .str = {S4_STR S1_STR}, .repr = {S4_REPR S1_REPR}},
    {.bytes = 6, .chars = 2, .name = {"s42"}, .str = {S4_STR S2_STR}, .repr = {S4_REPR S2_REPR}},
    {.bytes = 7, .chars = 2, .name = {"s43"}, .str = {S4_STR S3_STR}, .repr = {S4_REPR S3_REPR}},
    {.bytes = 8, .chars = 2, .name = {"s44"}, .str = {S4_STR S4_STR}, .repr = {S4_REPR S4_REPR}},
};

const char_t b1_1[] = {"\x80"};
const char_t b1_2[] = {"\xA0"};
const char_t b2[] = {"\xDA\xFF"};
const char_t b3[] = {"\xEA\xBF\xC0"};
const char_t b4[] = {"\xF2\xBF\x80\x25"};

Test(Function, utf8_measure)
{
    int32_t ret = 0;

    ret = utf8_measure(s1.str);
    cr_expect(ret == s1.bytes, "%s: utf8_measure('%s'): expect %d, got %d", s1.name, s1.repr, s1.bytes, ret);

    ret = utf8_measure(s2.str);
    cr_expect(ret == s2.bytes, "%s: utf8_measure('%s'): expect %d, got %d", s2.name, s2.repr, s2.bytes, ret);

    ret = utf8_measure(s3.str);
    cr_expect(ret == s3.bytes, "%s: utf8_measure('%s'): expect %d, got %d", s3.name, s3.repr, s3.bytes, ret);

    ret = utf8_measure(s4.str);
    cr_expect(ret == s4.bytes, "%s: utf8_measure('%s'): expect %d, got %d", s4.name, s4.repr, s4.bytes, ret);

    ret = utf8_measure(b1_1);
    cr_expect(ret == 0, "%s: utf8_measure('\\x80'): expect %d, got %d", "b1_1", 0, ret);

    ret = utf8_measure(b1_2);
    cr_expect(ret == 0, "%s: utf8_measure('\\xA0'): expect %d, got %d", "b1_2", 0, ret);
} // utf_measure

Test(Function, utf8_count)
{
    int32_t bytes = 0;
    int32_t chars = 0;
    int32_t i = 0;

    bytes = utf8_count(s1.str, s1.bytes, &chars);
    cr_expect(bytes == s1.bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", s1.name, s1.repr, s1.bytes, bytes);
    cr_expect(chars == s1.chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", s1.name, s1.repr, s1.chars, chars);

    chars = s2.chars - 1;
    bytes = utf8_count(s2.str, s2.bytes, &chars);
    cr_expect(bytes == s2.bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", s2.name, s2.repr, s2.bytes, bytes);
    cr_expect(chars == s2.chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", s2.name, s2.repr, s2.chars, chars);

    chars = s3.chars;
    bytes = utf8_count(s3.str, s3.bytes, &chars);
    cr_expect(bytes == s3.bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", s3.name, s3.repr, s3.bytes, bytes);
    cr_expect(chars == s3.chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", s3.name, s3.repr, s3.chars, chars);

    chars = s4.chars + 1;
    bytes = utf8_count(s4.str, s4.bytes, &chars);
    cr_expect(bytes == s4.bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", s4.name, s4.repr, s4.bytes, bytes);
    cr_expect(chars == s4.chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", s4.name, s4.repr, s4.chars, chars);

    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        chars = sc[i].chars;
        bytes = utf8_count(sc[i].str, sc[i].bytes, &chars);
        cr_expect(bytes == sc[i].bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", sc[i].name, sc[i].repr, sc[i].bytes, bytes);
        cr_expect(chars == sc[i].chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", sc[i].name, sc[i].repr, sc[i].chars, chars);
    } // for
} // utf8_count
