#include <criterion/criterion.h>

#ifndef UTF8_SOURCE
#define UTF8_SOURCE 1
#include "str/utf8.c"
#endif

typedef struct TEST_STR_DATA {
    int32_t bytes;
    int32_t chars;
    const char_t name[80];
    const char_t str[80];
    const char_t repr[80];
} test_str_data_t;

#define S1_STR "A"
#define S1_REPR "A"
#define S2_STR "\xDA\xBF"
#define S2_REPR "\\xDA\\xBF"
#define S3_STR "\xEA\xBF\x80"
#define S3_REPR "\\xEA\\xBF\\x80"
#define S4_STR "\xF2\xBF\x80\xA5"
#define S4_REPR "\\xF2\\xBF\\x80\\xA5"

test_str_data_t s1 = {.bytes = 1, .chars = 1, .name = {"s1"}, .str = {S1_STR}, .repr = {S1_REPR}};
test_str_data_t s2 = {.bytes = 2, .chars = 1, .name = {"s2"}, .str = {S2_STR}, .repr = {S2_REPR}};
test_str_data_t s3 = {.bytes = 3, .chars = 1, .name = {"s3"}, .str = {S3_STR}, .repr = {S3_REPR}};
test_str_data_t s4 = {.bytes = 4, .chars = 1, .name = {"s4"}, .str = {S4_STR}, .repr = {S4_REPR}};

test_str_data_t sc[] = {
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
const char_t b1_3[] = {"\xF8"};
const char_t b1_4[] = {"\xFF"};
const char_t b2[] = {"\xDA\xFF"};
const char_t b3[] = {"\xEA\xBF\xC0"};

#define B4_STR "\xF2\xBF\x80\x25"
#define B4_REPR "\\xF2\\xBF\\x80\\x25"

test_str_data_t b4 = {.bytes = 4, .chars = 1, .name = {"b4"}, .str = {B4_STR}, .repr = {B4_REPR}};

#define BZ_STR "\x92\xBF\x80\xA5"
#define BZ_REPR "\\x92\\xBF\\x80\\xA5"

test_str_data_t bz = {.bytes = 4, .chars = 1, .name = {"bz"}, .str = {BZ_STR}, .repr = {BZ_REPR}};

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

    ret = utf8_measure(b1_3);
    cr_expect(ret == -1, "%s: utf8_measure('\\xF8'): expect %d, got %d", "b1_3", -1, ret);

    ret = utf8_measure(b1_4);
    cr_expect(ret == -1, "%s: utf8_measure('\\xFF'): expect %d, got %d", "b1_4", -1, ret);
} // utf_measure

Test(Function, utf8_count)
{
    int32_t bytes = 0;
    int32_t chars = 0;
    int32_t i = 0;

    chars = s1.chars;
    bytes = utf8_count(s1.str, s1.bytes, &chars);
    cr_expect(bytes == s1.bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", s1.name, s1.repr, s1.bytes, bytes);
    cr_expect(chars == s1.chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", s1.name, s1.repr, s1.chars, chars);

    chars = s2.chars;
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

    chars = b4.chars;
    bytes = utf8_count(b4.str, b4.bytes, &chars);
    cr_expect(bytes == -1, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", b4.name, b4.repr, -1, bytes);
    cr_expect(chars == 0, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", b4.name, b4.repr, 0, chars);

    chars = bz.chars;
    bytes = utf8_count(bz.str, bz.bytes, &chars);
    cr_expect(bytes == -1, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", bz.name, bz.repr, -1, bytes);
    cr_expect(chars == 0, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", bz.name, bz.repr, 0, chars);
} // utf8_count

Test(Function, utf8_verify)
{
    int32_t i = 0;
    bool ret = false;

    ret = utf8_verify(s1.str, s1.bytes);
    cr_expect(ret == true, "%s: utf8_verify('%s') return incorrect result: expect %d, got %d", s1.name, s1.repr, true, ret);

    ret = utf8_verify(s2.str, s2.bytes);
    cr_expect(ret == true, "%s: utf8_verify('%s') return incorrect result: expect %d, got %d", s2.name, s2.repr, true, ret);

    ret = utf8_verify(s3.str, s3.bytes);
    cr_expect(ret == true, "%s: utf8_verify('%s') return incorrect result: expect %d, got %d", s3.name, s3.repr, true, ret);

    ret = utf8_verify(s4.str, s4.bytes);
    cr_expect(ret == true, "%s: utf8_verify('%s') return incorrect result: expect %d, got %d", s4.name, s4.repr, true, ret);

    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        ret = utf8_verify(sc[i].str, sc[i].bytes);
        cr_expect(ret == true, "%s: utf8_verify('%s') return incorrect result: expect %d, got %d", sc[i].name, sc[i].repr, true, ret);
    } // for
} // utf8_verify

typedef struct TEST_UNICODE_DATA {
    uchar_t ch;
    int32_t bytes;
    char_t seq[8];
    char_t repr[80];
} test_unicode_data_t;

test_unicode_data_t ug[] = {
    {.ch = 0x000000, .bytes = 1, .seq = {"\x00"}, .repr = {"\\x00"}},
    {.ch = 0x00007F, .bytes = 1, .seq = {"\x7F"}, .repr = {"\\x7f"}},
    {.ch = 0x000080, .bytes = 2, .seq = {"\xC2\x80"}, .repr = {"\\xC2\\x80"}},
    {.ch = 0x0003A9, .bytes = 2, .seq = {"\xCE\xA9"}, .repr = {"\\xCE\\xA9"}},
    {.ch = 0x0007FF, .bytes = 2, .seq = {"\xDF\xBF"}, .repr = {"\\xDF\\xBF"}},
    {.ch = 0x000800, .bytes = 3, .seq = {"\xE0\xA0\x80"}, .repr = {"\\xE0\\xA0\\x80"}},
    {.ch = 0x005AD0, .bytes = 3, .seq = {"\xE5\xAB\x90"}, .repr = {"\\xE5\\xAB\\x90"}},
    {.ch = 0x00FFFF, .bytes = 3, .seq = {"\xEF\xBF\xBF"}, .repr = {"\\xEF\\xBF\\xBF"}},
    {.ch = 0x010000, .bytes = 4, .seq = {"\xF0\x90\x80\x80"}, .repr = {"\\xF0\\x90\\x80\\x80"}},
    {.ch = 0x10FFFF, .bytes = 4, .seq = {"\xF4\x8F\xBF\xBF"}, .repr = {"\\xF4\\x8F\\xBF\\xBF"}},
};

Test(Function, utf8_decode)
{
    uchar_t ch = 0;
    int i = 0;

    for (i = 0; i < sizeof(ug) / sizeof(ug[0]); ++i) {
        ch = utf8_decode(ug[i].seq);
        cr_expect(ch == ug[i].ch, "utf8_decode(%s) return incorrect ch: expect %06X, got %06X", ug[i].repr, ug[i].ch, ch);
    } // for
} // utf8_decode

Test(Function, utf8_encode)
{
    char_t seq[4] = {0};
    int32_t bytes = 0;
    int i = 0;
    int j = 0;

    for (i = 0; i < sizeof(ug) / sizeof(ug[0]); ++i) {
        memset(seq, 0, sizeof(seq));
        bytes = utf8_encode(ug[i].ch, seq);
        cr_expect(bytes == ug[i].bytes, "utf8_encode(0x%06X) return incorrect bytes: expect %d, got %d", ug[i].ch, ug[i].bytes, bytes);
        for (j = 0; j < ug[i].bytes; ++j) {
            cr_expect(seq[j] == ug[i].seq[j], "utf8_encode(0x%06X) return incorrect seq[%d]: expect %02X, got %02X", ug[i].ch, j, ug[i].seq[j], seq[j]);
        } // for
    } // for
} // utf8_encode
