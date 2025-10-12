#include <criterion/criterion.h>

#ifndef UTF8_SOURCE
#define UTF8_SOURCE 1
#include "str/utf8.c"
#endif

typedef struct UT_STRING_CASE {
    uint32_t bytes;
    uint32_t chars;
    uint32_t i_bytes;
    uint32_t i_chars;
    uint32_t r_bytes;
    uint32_t r_chars;
    uint32_t measure_ret;
    bool count_ret;
    const char_t name[80];
    const char_t str[80];
    const char_t repr[80];
} ut_string_case_t, *ut_string_case_p;

#define S1_STR "A" // 0b00001010
#define S1_REPR "A"
#define S2_STR "\xDA\xBF" // 0b11011010 0b10111111
#define S2_REPR "\\xDA\\xBF"
#define S3_STR "\xEA\xBF\x80" // 0b11101010 0b10111111 0b10000000
#define S3_REPR "\\xEA\\xBF\\x80"
#define S4_STR "\xF2\xBF\x80\xA5" // 0b11110010 0b10111111 0b10000000 0b10100101
#define S4_REPR "\\xF2\\xBF\\x80\\xA5"

static ut_string_case_t sc[] = {
    {.name = {"s1"}, .bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 1, .r_chars = 1, .measure_ret = 1, .count_ret = true, .str = {S1_STR}, .repr = {S1_REPR}},
    {.name = {"s2"}, .bytes = 2, .chars = 1, .i_bytes = 2, .i_chars = 1, .r_bytes = 2, .r_chars = 1, .measure_ret = 2, .count_ret = true, .str = {S2_STR}, .repr = {S2_REPR}},
    {.name = {"s3"}, .bytes = 3, .chars = 1, .i_bytes = 3, .i_chars = 1, .r_bytes = 3, .r_chars = 1, .measure_ret = 3, .count_ret = true, .str = {S3_STR}, .repr = {S3_REPR}},
    {.name = {"s4"}, .bytes = 4, .chars = 1, .i_bytes = 4, .i_chars = 1, .r_bytes = 4, .r_chars = 1, .measure_ret = 4, .count_ret = true, .str = {S4_STR}, .repr = {S4_REPR}},

    {.name = {"s11"}, .bytes = 2, .chars = 2, .i_bytes = 2, .i_chars = 2, .r_bytes = 2, .r_chars = 2, .measure_ret = 1, .count_ret = true, .str = {S1_STR S1_STR}, .repr = {S1_REPR S1_REPR}},
    {.name = {"s12"}, .bytes = 3, .chars = 2, .i_bytes = 3, .i_chars = 2, .r_bytes = 3, .r_chars = 2, .measure_ret = 1, .count_ret = true, .str = {S1_STR S2_STR}, .repr = {S1_REPR S2_REPR}},
    {.name = {"s13"}, .bytes = 4, .chars = 2, .i_bytes = 4, .i_chars = 2, .r_bytes = 4, .r_chars = 2, .measure_ret = 1, .count_ret = true, .str = {S1_STR S3_STR}, .repr = {S1_REPR S3_REPR}},
    {.name = {"s14"}, .bytes = 5, .chars = 2, .i_bytes = 5, .i_chars = 2, .r_bytes = 5, .r_chars = 2, .measure_ret = 1, .count_ret = true, .str = {S1_STR S4_STR}, .repr = {S1_REPR S4_REPR}},
                      
    {.name = {"s21"}, .bytes = 3, .chars = 2, .i_bytes = 3, .i_chars = 2, .r_bytes = 3, .r_chars = 2, .measure_ret = 2, .count_ret = true, .str = {S2_STR S1_STR}, .repr = {S2_REPR S1_REPR}},
    {.name = {"s22"}, .bytes = 4, .chars = 2, .i_bytes = 4, .i_chars = 2, .r_bytes = 4, .r_chars = 2, .measure_ret = 2, .count_ret = true, .str = {S2_STR S2_STR}, .repr = {S2_REPR S2_REPR}},
    {.name = {"s23"}, .bytes = 5, .chars = 2, .i_bytes = 5, .i_chars = 2, .r_bytes = 5, .r_chars = 2, .measure_ret = 2, .count_ret = true, .str = {S2_STR S3_STR}, .repr = {S2_REPR S3_REPR}},
    {.name = {"s24"}, .bytes = 6, .chars = 2, .i_bytes = 6, .i_chars = 2, .r_bytes = 6, .r_chars = 2, .measure_ret = 2, .count_ret = true, .str = {S2_STR S4_STR}, .repr = {S2_REPR S4_REPR}},
                                                                                                  
    {.name = {"s31"}, .bytes = 4, .chars = 2, .i_bytes = 4, .i_chars = 2, .r_bytes = 4, .r_chars = 2, .measure_ret = 3, .count_ret = true, .str = {S3_STR S1_STR}, .repr = {S3_REPR S1_REPR}},
    {.name = {"s32"}, .bytes = 5, .chars = 2, .i_bytes = 5, .i_chars = 2, .r_bytes = 5, .r_chars = 2, .measure_ret = 3, .count_ret = true, .str = {S3_STR S2_STR}, .repr = {S3_REPR S2_REPR}},
    {.name = {"s33"}, .bytes = 6, .chars = 2, .i_bytes = 6, .i_chars = 2, .r_bytes = 6, .r_chars = 2, .measure_ret = 3, .count_ret = true, .str = {S3_STR S3_STR}, .repr = {S3_REPR S3_REPR}},
    {.name = {"s34"}, .bytes = 7, .chars = 2, .i_bytes = 7, .i_chars = 2, .r_bytes = 7, .r_chars = 2, .measure_ret = 3, .count_ret = true, .str = {S3_STR S4_STR}, .repr = {S3_REPR S4_REPR}},
                                                                                                  
    {.name = {"s41"}, .bytes = 5, .chars = 2, .i_bytes = 5, .i_chars = 2, .r_bytes = 5, .r_chars = 2, .measure_ret = 4, .count_ret = true, .str = {S4_STR S1_STR}, .repr = {S4_REPR S1_REPR}},
    {.name = {"s42"}, .bytes = 6, .chars = 2, .i_bytes = 6, .i_chars = 2, .r_bytes = 6, .r_chars = 2, .measure_ret = 4, .count_ret = true, .str = {S4_STR S2_STR}, .repr = {S4_REPR S2_REPR}},
    {.name = {"s43"}, .bytes = 7, .chars = 2, .i_bytes = 7, .i_chars = 2, .r_bytes = 7, .r_chars = 2, .measure_ret = 4, .count_ret = true, .str = {S4_STR S3_STR}, .repr = {S4_REPR S3_REPR}},
    {.name = {"s44"}, .bytes = 8, .chars = 2, .i_bytes = 8, .i_chars = 2, .r_bytes = 8, .r_chars = 2, .measure_ret = 4, .count_ret = true, .str = {S4_STR S4_STR}, .repr = {S4_REPR S4_REPR}},
};

#define B1_1_STR "\x80"                     // 0b10000000
#define B1_1_REPR "\\x80"                   //   ^^
#define B1_2_STR "\xA0"                     // 0b10100000
#define B1_2_REPR "\\xA0"                   //   ^^
#define B1_3_STR "\xF8"                     // 0b11111000
#define B1_3_REPR "\\xF8"                   //   ^^^^
#define B1_4_STR "\xFF"                     // 0b11111111
#define B1_4_REPR "\\xFF"                   //   ^^^^
#define B1_5_STR "\xFA\xFF"                 // 0b11111010 0b11111111
#define B1_5_REPR "\\xFA\\xFF"              //   ^^^^
#define B2_STR "\xBA\xFF"                   // 0b10111010 0b11111111
#define B2_REPR "\\xBA\\xFF"                //   ^^         ^^
#define B3_STR "\xEA\xBF\xC0"               // 0b11101010 0b10111111 0b11000000
#define B3_REPR "\\xEA\\xBF\\xC0"           //                         ^^
#define B4_STR "\xF2\xBF\x80\x25"           // 0b11110010 0b10111111 0b10000000 0b00100101
#define B4_REPR "\\xF2\\xBF\\x80\\x25"      //                                    ^^
#define BZ_STR "\x92\xBF\x80\xA5"           // 0b10010010 0b10111111 0b10000000 0b10100101
#define BZ_REPR "\\x92\\xBF\\x80\\xA5"      //   ^^

static ut_string_case_t bc[] = {
    {.name = {"b1-1"}, .bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {B1_1_STR}, .repr = {B1_1_REPR}},
    {.name = {"b1-2"}, .bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {B1_2_STR}, .repr = {B1_2_REPR}},
    {.name = {"b1-3"}, .bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {B1_3_STR}, .repr = {B1_3_REPR}},
    {.name = {"b1-4"}, .bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {B1_4_STR}, .repr = {B1_4_REPR}},
    {.name = {"b1-5"}, .bytes = 1, .chars = 1, .i_bytes = 1, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {B1_5_STR}, .repr = {B1_5_REPR}},

    {.name = {"b2"}, .bytes = 2, .chars = 1, .i_bytes = 2, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {B2_STR}, .repr = {B2_REPR}},
    {.name = {"b3"}, .bytes = 3, .chars = 1, .i_bytes = 3, .i_chars = 1, .r_bytes = 2, .r_chars = 0, .measure_ret = 3, .count_ret = false, .str = {B3_STR}, .repr = {B3_REPR}},
    {.name = {"b4"}, .bytes = 4, .chars = 1, .i_bytes = 4, .i_chars = 1, .r_bytes = 3, .r_chars = 0, .measure_ret = 4, .count_ret = false, .str = {B4_STR}, .repr = {B4_REPR}},
    {.name = {"bz"}, .bytes = 4, .chars = 1, .i_bytes = 4, .i_chars = 1, .r_bytes = 0, .r_chars = 0, .measure_ret = 0, .count_ret = false, .str = {BZ_STR}, .repr = {BZ_REPR}},
};

Test(Function, utf8_measure_plain)
{
    ut_string_case_p c = NULL;
    int i = 0;
    uint32_t ret = 0;

    // 正常用例
    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        c = &sc[i];
        ret = utf8_measure_plain(c->str);
        cr_expect(ret == c->measure_ret, "%s: utf8_measure_plain('%s') returns incorrect result: expect %d, got %d", c->name, c->repr, c->measure_ret, ret);
    } // for

    // 异常用例
    for (i = 0; i < sizeof(bc) / sizeof(bc[0]); ++i) {
        c = &bc[i];
        ret = utf8_measure_plain(c->str);
        cr_expect(ret == c->measure_ret, "%s: utf8_measure_plain('%s') returns incorrect result: expect %d, got %d", c->name, c->repr, c->measure_ret, ret);
    } // for
} // utf8_measure_plain

Test(Function, utf8_measure_by_lookup)
{
    ut_string_case_p c = NULL;
    int i = 0;
    uint32_t ret = 0;

    // 正常用例
    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        c = &sc[i];
        ret = utf8_measure_by_lookup(c->str);
        cr_expect(ret == c->measure_ret, "%s: utf8_measure_by_lookup('%s') returns incorrect result: expect %d, got %d", c->name, c->repr, c->measure_ret, ret);
    } // for

    // 异常用例
    for (i = 0; i < sizeof(bc) / sizeof(bc[0]); ++i) {
        c = &bc[i];
        ret = utf8_measure_by_lookup(c->str);
        cr_expect(ret == c->measure_ret, "%s: utf8_measure_by_lookup('%s') returns incorrect result: expect %d, got %d", c->name, c->repr, c->measure_ret, ret);
    } // for
} // utf8_measure_by_lookup

Test(Function, utf8_measure_by_addup)
{
    ut_string_case_p c = NULL;
    int i = 0;
    uint32_t ret = 0;

    // 正常用例
    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        c = &sc[i];
        ret = utf8_measure_by_addup(c->str);
        cr_expect(ret == c->measure_ret, "%s: utf8_measure_by_addup('%s') returns incorrect result: expect %d, got %d", c->name, c->repr, c->measure_ret, ret);
    } // for

    // 异常用例
    for (i = 0; i < sizeof(bc) / sizeof(bc[0]); ++i) {
        c = &bc[i];
        ret = utf8_measure_by_addup(c->str);
        cr_expect(ret == c->measure_ret, "%s: utf8_measure_by_addup('%s') returns incorrect result: expect %d, got %d", c->name, c->repr, c->measure_ret, ret);
    } // for
} // utf8_measure_by_addup

Test(Function, utf8_count)
{
    ut_string_case_p c = NULL;
    int32_t i = 0;
    uint32_t r_bytes = 0;
    uint32_t r_chars = 0;
    bool ret = false;

    // 正常用例
    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        c= &sc[i];
        r_bytes = c->i_bytes;
        r_chars = c->i_chars;
        ret = utf8_count(c->str, &r_bytes, &r_chars);
        cr_expect(r_bytes == c->r_bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", c->name, c->repr, c->r_bytes, r_bytes);
        cr_expect(r_chars == c->r_chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", c->name, c->repr, c->r_chars, r_chars);
        cr_expect(ret == c->count_ret, "%s: utf8_count('%s') return incorrect result: expect %d, got %d", c->name, c->repr, c->count_ret, ret);
    } // for

    // 异常用例
    for (i = 0; i < sizeof(bc) / sizeof(bc[0]); ++i) {
        c= &bc[i];
        r_bytes = c->i_bytes;
        r_chars = c->i_chars;
        ret = utf8_count(c->str, &r_bytes, &r_chars);
        cr_expect(r_bytes == c->r_bytes, "%s: utf8_count('%s') return incorrect bytes: expect %d, got %d", c->name, c->repr, c->r_bytes, r_bytes);
        cr_expect(r_chars == c->r_chars, "%s: utf8_count('%s') return incorrect chars: expect %d, got %d", c->name, c->repr, c->r_chars, r_chars);
        cr_expect(ret == c->count_ret, "%s: utf8_count('%s') return incorrect result: expect %d, got %d", c->name, c->repr, c->count_ret, ret);
    } // for
} // utf8_count

typedef struct TEST_UNICODE_DATA {
    uchar_t ch;
    int32_t bytes;
    char_t seq[8];
    char_t repr[80];
} test_unicode_data_t;

test_unicode_data_t ug[] = {
    {.ch = 0x000000, .bytes = 1, .seq = {"\x00"}, .repr = {"\\x00"}},
    {.ch = 0x00007F, .bytes = 1, .seq = {"\x7F"}, .repr = {"\\x7F"}},
    {.ch = 0x000080, .bytes = 2, .seq = {"\xC2\x80"}, .repr = {"\\xC2\\x80"}},
    {.ch = 0x0003A9, .bytes = 2, .seq = {"\xCE\xA9"}, .repr = {"\\xCE\\xA9"}},
    {.ch = 0x0007FF, .bytes = 2, .seq = {"\xDF\xBF"}, .repr = {"\\xDF\\xBF"}},
    {.ch = 0x000800, .bytes = 3, .seq = {"\xE0\xA0\x80"}, .repr = {"\\xE0\\xA0\\x80"}},
    {.ch = 0x005AD0, .bytes = 3, .seq = {"\xE5\xAB\x90"}, .repr = {"\\xE5\\xAB\\x90"}},
    {.ch = 0x00FFFF, .bytes = 3, .seq = {"\xEF\xBF\xBF"}, .repr = {"\\xEF\\xBF\\xBF"}},
    {.ch = 0x010000, .bytes = 4, .seq = {"\xF0\x90\x80\x80"}, .repr = {"\\xF0\\x90\\x80\\x80"}},
    {.ch = 0x0F0000, .bytes = 4, .seq = {"\xF3\xB0\x80\x80"}, .repr = {"\\xF3\\xB0\\x80\\x80"}},
    {.ch = 0x10FFFF, .bytes = 4, .seq = {"\xF4\x8F\xBF\xBF"}, .repr = {"\\xF4\\x8F\\xBF\\xBF"}},
};

test_unicode_data_t ub[] = {
    {.ch = ~0L, .bytes = -1, .seq = {B1_1_STR}, .repr = {B1_1_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {B1_2_STR}, .repr = {B1_2_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {B1_3_STR}, .repr = {B1_3_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {B1_4_STR}, .repr = {B1_4_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {B2_STR}, .repr = {B2_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {B3_STR}, .repr = {B3_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {B4_STR}, .repr = {B4_REPR}},
    {.ch = ~0L, .bytes = -1, .seq = {BZ_STR}, .repr = {BZ_REPR}},
};

Test(Function, utf8_decode)
{
    uchar_t ch = 0;
    int32_t bytes = 0;
    int i = 0;

    // 正常用例
    for (i = 0; i < sizeof(ug) / sizeof(ug[0]); ++i) {
        bytes = utf8_decode(ug[i].seq, &ch);
        cr_expect(ch == ug[i].ch, "ug[%d] utf8_decode(%s) return incorrect ch: expect 0x%06X, got 0x%06X", i, ug[i].repr, ug[i].ch, ch);
        cr_expect(bytes == ug[i].bytes, "ug[%d] utf8_decode(%s) return incorrect bytes: expect %d, got %d", i, ug[i].repr, ug[i].bytes, bytes);
    } // for

    // 异常用例
    for (i = 0; i < sizeof(ub) / sizeof(ub[0]); ++i) {
        ch = ~0L;
        bytes = utf8_decode(ub[i].seq, &ch);
        cr_expect(ch == ub[i].ch, "ub[%d] utf8_decode(%s) return incorrect ch: expect 0x%X, got 0x%X", i, ub[i].repr, ub[i].ch, ch);
        cr_expect(bytes == ub[i].bytes, "ub[%d] utf8_decode(%s) return incorrect bytes: expect %d, got %d", i, ub[i].repr, ub[i].bytes, bytes);
    } // for
} // utf8_decode

Test(Function, utf8_encode)
{
    char_t seq[4] = {0};
    int32_t bytes = 0;
    int i = 0;
    int j = 0;

    // 正常用例
    for (i = 0; i < sizeof(ug) / sizeof(ug[0]); ++i) {
        memset(seq, 0, sizeof(seq));
        bytes = utf8_encode(ug[i].ch, seq);
        cr_expect(bytes == ug[i].bytes, "utf8_encode(0x%06X) return incorrect bytes: expect %d, got %d", ug[i].ch, ug[i].bytes, bytes);
        for (j = 0; j < ug[i].bytes; ++j) {
            cr_expect(seq[j] == ug[i].seq[j], "utf8_encode(0x%06X) return incorrect seq[%d]: expect %02X, got %02X", ug[i].ch, j, ug[i].seq[j], seq[j]);
        } // for
    } // for
} // utf8_encode
