#include <criterion/criterion.h>

#ifndef MISC_SOURCE
#define MISC_SOURCE 1
#include "str/misc.c"
#endif

typedef struct UT_SPAN_CASE {
    const char_t * start;
    uint32_t bytes;
    uint32_t alignment;
    uint32_t r_leads;
    uint32_t r_chunks;
    uint32_t r_tails;
    const char_t name[80];
} ut_span_t, *ut_span_p;

static ut_span_t sc[] = {
    {.name = {"blank"},        .start = (const char_t *)0x0, .bytes = 0,  .alignment = 8, .r_leads = 0, .r_chunks = 0, .r_tails = 0},
    {.name = {"one"},          .start = (const char_t *)0x3, .bytes = 1,  .alignment = 8, .r_leads = 3, .r_chunks = 0, .r_tails = 4},
    {.name = {"two"},          .start = (const char_t *)0x4, .bytes = 2,  .alignment = 8, .r_leads = 4, .r_chunks = 0, .r_tails = 2},
    {.name = {"leads"},        .start = (const char_t *)0x5, .bytes = 3,  .alignment = 8, .r_leads = 5, .r_chunks = 0, .r_tails = 0},
    {.name = {"tails"},        .start = (const char_t *)0x0, .bytes = 6,  .alignment = 8, .r_leads = 0, .r_chunks = 0, .r_tails = 2},
    {.name = {"full"},         .start = (const char_t *)0x0, .bytes = 8,  .alignment = 8, .r_leads = 0, .r_chunks = 1, .r_tails = 0},
    {.name = {"cross"},        .start = (const char_t *)0x7, .bytes = 2,  .alignment = 8, .r_leads = 7, .r_chunks = 0, .r_tails = 7},
    {.name = {"cross_leads"},  .start = (const char_t *)0x6, .bytes = 10, .alignment = 8, .r_leads = 6, .r_chunks = 1, .r_tails = 0},
    {.name = {"cross_tails"},  .start = (const char_t *)0x0, .bytes = 10, .alignment = 8, .r_leads = 0, .r_chunks = 1, .r_tails = 6},
    {.name = {"cross_full"},   .start = (const char_t *)0x0, .bytes = 16, .alignment = 8, .r_leads = 0, .r_chunks = 2, .r_tails = 0},
    {.name = {"cross2"},       .start = (const char_t *)0x7, .bytes = 10, .alignment = 8, .r_leads = 7, .r_chunks = 1, .r_tails = 7},
    {.name = {"cross2_leads"}, .start = (const char_t *)0x6, .bytes = 18, .alignment = 8, .r_leads = 6, .r_chunks = 2, .r_tails = 0},
    {.name = {"cross2_tails"}, .start = (const char_t *)0x0, .bytes = 18, .alignment = 8, .r_leads = 0, .r_chunks = 2, .r_tails = 6},
    {.name = {"cross2_full"},  .start = (const char_t *)0x0, .bytes = 24, .alignment = 8, .r_leads = 0, .r_chunks = 3, .r_tails = 0},
};

static void str_span_wrapper(const char_t * start, const uint32_t bytes, const uint32_t alignment, uint32_t * leads, uint32_t * chunks, uint32_t * tails)
{
    str_span(start, bytes, alignment, leads, chunks, tails);
} // str_span_wrapper

Test(Function, str_span)
{
    ut_span_p c = NULL;
    uint32_t r_leads = 0;
    uint32_t r_chunks = 0;
    uint32_t r_tails = 0;
    int i = 0;

    // 正常用例
    for (i = 0; i < sizeof(sc) / sizeof(sc[0]); ++i) {
        c = &sc[i];
        str_span_wrapper(c->start, c->bytes, c->alignment, &r_leads, &r_chunks, &r_tails);
        cr_expect(r_leads == c->r_leads, "%s: str_span(%p, %u, %u) returns incorrect leads: expect %u, got %u", c->name, c->start, c->bytes, c->alignment, c->r_leads, r_leads);
        cr_expect(r_chunks == c->r_chunks, "%s: str_span(%p, %u, %u) returns incorrect chunks: expect %u, got %u", c->name, c->start, c->bytes, c->alignment, c->r_chunks, r_chunks);
        cr_expect(r_tails == c->r_tails, "%s: str_span(%p, %u, %u) returns incorrect tails: expect %u, got %u", c->name, c->start, c->bytes, c->alignment, c->r_tails, r_tails);
    } // for
} // str_span
