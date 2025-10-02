#include <criterion/criterion.h>

#ifndef ASCII_SOURCE
#define ASCII_SOURCE 1
#include "str/ascii.c"
#endif

typedef struct UT {
    uint32_t        bytes;
    uint32_t        chars;
    uint32_t        i_bytes;
    uint32_t        i_chars;
    uint32_t        r_bytes;
    uint32_t        r_chars;
    bool            result;
    const char_t    name[80];
    const char_t    str[80];
    const char_t    repr[80];
} ut_t;

#define S1_STR "A"
#define S1_REPR "A"
#define S2_STR "AB"
#define S2_REPR "AB"
#define S3_STR "\x20\x40\x7F"
#define S3_REPR "\\x20\\x40\\x7F"

#define B1_STR "\0"
#define B1_REPR "\\0"
#define B2_STR "A\x80"
#define B2_REPR "A\\x80"
#define B3_STR "AB" "\xA0"
#define B3_REPR "AB" "\\xA0"
#define B4_STR "\xE0" "A"
#define B4_REPR "\\xE0" "A"
#define B5_STR "B\xE0" "A"
#define B5_REPR "B\\xE0" "A"

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

    ch = '\xE0';
    cr_expect(ascii_measure(&ch) == 0, "ascii_measure('\\xE0') return 1");

    ch = '\xFF';
    cr_expect(ascii_measure(&ch) == 0, "ascii_measure('\\xFF') return 1");
} // ascii_measure

Test(Function, ascii_count)
{
    static ut_t sc[] = {
        {.bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 1, .r_chars = 1, .result = true, .name = "s1", .str = {S1_STR}, .repr = {S1_REPR}},
        {.bytes = 2, .chars = 2, .i_bytes = 2, .i_chars = 2, .r_bytes = 2, .r_chars = 2, .result = true, .name = "s2", .str = {S2_STR}, .repr = {S2_REPR}},
        {.bytes = 3, .chars = 3, .i_bytes = 3, .i_chars = 3, .r_bytes = 3, .r_chars = 3, .result = true, .name = "s3", .str = {S3_STR}, .repr = {S3_REPR}},

        {.bytes = 3, .chars = 3, .i_bytes = 3, .i_chars = 1, .r_bytes = 1, .r_chars = 1, .result = true, .name = "s3_1", .str = {S3_STR}, .repr = {S3_REPR}},
        {.bytes = 3, .chars = 3, .i_bytes = 2, .i_chars = 3, .r_bytes = 2, .r_chars = 2, .result = true, .name = "s3_2", .str = {S3_STR}, .repr = {S3_REPR}},
        {.bytes = 3, .chars = 3, .i_bytes = 0, .i_chars = 3, .r_bytes = 0, .r_chars = 0, .result = true, .name = "s3_3", .str = {S3_STR}, .repr = {S3_REPR}},
        {.bytes = 3, .chars = 3, .i_bytes = 3, .i_chars = 0, .r_bytes = 0, .r_chars = 0, .result = true, .name = "s3_4", .str = {S3_STR}, .repr = {S3_REPR}},

        {.bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .result = false, .name = "b1", .str = {B1_STR}, .repr = {B1_REPR}},
        {.bytes = 2, .chars = 2, .i_bytes = 2, .i_chars = 2, .r_bytes = 1, .r_chars = 1, .result = false, .name = "b2", .str = {B2_STR}, .repr = {B2_REPR}},
        {.bytes = 3, .chars = 3, .i_bytes = 3, .i_chars = 3, .r_bytes = 2, .r_chars = 2, .result = false, .name = "b3", .str = {B3_STR}, .repr = {B3_REPR}},
        {.bytes = 2, .chars = 2, .i_bytes = 2, .i_chars = 2, .r_bytes = 0, .r_chars = 0, .result = false, .name = "b4", .str = {B4_STR}, .repr = {B4_REPR}},
        {.bytes = 3, .chars = 3, .i_bytes = 3, .i_chars = 3, .r_bytes = 1, .r_chars = 1, .result = false, .name = "b5", .str = {B5_STR}, .repr = {B5_REPR}},
    };

    int i = 0;
    uint32_t r_bytes = 0;
    uint32_t r_chars = 0;
    bool ret = false;

    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        r_bytes = sc[i].i_bytes;
        r_chars = sc[i].i_chars;
        ret = ascii_count(sc[i].str, &r_bytes, &r_chars);
        cr_expect(r_bytes == sc[i].r_bytes, "ascii_count(%s) return incorrect bytes: expect %d, got %d", sc[i].name, sc[i].r_bytes, r_bytes);
        cr_expect(r_chars == sc[i].r_chars, "ascii_count(%s) return incorrect chars: expect %d, got %d", sc[i].name, sc[i].r_chars, r_chars);
        cr_expect(ret == sc[i].result, "ascii_count(%s) return incorrect result: expect %d, got %d", sc[i].name, sc[i].result, ret);
    } // for
/*
    int32_t size = 0;
    int32_t bytes = 0;
    int32_t chars = 0;
    bool ret = false;

    size = sizeof(s1) - 1;

    size = sizeof(s1) - 1;
    bytes = size;
    chars = size;
    ret = ascii_count(s1, &bytes, &chars);
    cr_expect(bytes == size, "ascii_count(s1) return incorrect bytes: expect %d, got %d", size, bytes);
    cr_expect(chars == size, "ascii_count(s1) return incorrect chars: expect %d, got %d", size, chars);
    cr_expect(ret == true, "ascii_count(s1) return incorrect result: expect %d, got %d", true, ret);

    size = sizeof(s2) - 1;
    bytes = size;
    chars = size;
    ret = ascii_count(s2, &bytes, &chars);
    cr_expect(bytes == size, "ascii_count(s2) return incorrect bytes: expect %d, got %d", size, bytes);
    cr_expect(chars == size, "ascii_count(s2) return incorrect chars: expect %d, got %d", size, chars);
    cr_expect(ret == true, "ascii_count(s2) return incorrect result: expect %d, got %d", true, ret);

    size = sizeof(s3) - 1;
    bytes = size;
    chars = size + 1;
    ret = ascii_count(s3, &bytes, &chars);
    cr_expect(bytes == size, "ascii_count(s3) return incorrect bytes: expect %d, got %d", size, bytes);
    cr_expect(chars == size, "ascii_count(s3) return incorrect chars: expect %d, got %d", size, chars);
    cr_expect(ret == true, "ascii_count(s3) return incorrect result: expect %d, got %d", true, ret);

    size = sizeof(b1) - 1;
    bytes = size;
    chars = size;
    ret = ascii_count(b1, &bytes, &chars);
    cr_expect(bytes == 0, "ascii_count(b1) return incorrect bytes: expect %d, got %d", 0, bytes);
    cr_expect(chars == 0, "ascii_count(b1) return incorrect chars: expect %d, got %d", 0, chars);
    cr_expect(ret == false, "ascii_count(b1) return incorrect result: expect %d, got %d", false, ret);

    size = sizeof(b2) - 1;
    bytes = size;
    chars = size;
    ascii_count(b2, &bytes, &chars);
    cr_expect(bytes == 1, "ascii_count(b2) return incorrect bytes: expect %d, got %d", 1, bytes);
    cr_expect(chars == 1, "ascii_count(b2) return incorrect chars: expect %d, got %d", 1, chars);
    cr_expect(ret == false, "ascii_count(b2) return incorrect result: expect %d, got %d", false, ret);

    size = sizeof(b3) - 1;
    bytes = size;
    chars = size;
    ascii_count(b3, &bytes, &chars);
    cr_expect(bytes == 0, "ascii_count(b3) return incorrect bytes: expect %d, got %d", 0, bytes);
    cr_expect(chars == 0, "ascii_count(b3) return incorrect chars: expect %d, got %d", 0, chars);
    cr_expect(ret == false, "ascii_count(b3) return incorrect result: expect %d, got %d", false, ret);
*/
} // ascii_count

