#include <criterion/criterion.h>

#ifndef NSTR_SOURCE
#define NSTR_SOURCE 1
#include "str/nstr.c"
#endif

Test(Creation, nstr_blank)
{
    nstr_p blk = nstr_blank();

    cr_expect(blk == &blank, "nstr_blank() return pointer: expect %p, got %p", &blank, blk);
    cr_expect(blank_dummy.slcs == 1, "nstr_blank() don't add references: expect %d, got %d", 1, blank_dummy.slcs);
    cr_expect(blank_dummy.need_free == 0);
    cr_expect(blank.need_free == 0);

    nstr_delete(blk);
} // nstr_blank

void check_slice(const char_t * func, nstr_p s, int32_t need_free, int32_t bytes, int32_t chars, str_encoding_t encoding, const char_t * start, int32_t offset, entity_p ent, int32_t slcs)
{
    cr_expect(s != NULL, "%s() return pointer: expect non-NULL, got NULL", func);
    cr_expect(s->need_free == need_free, "%s() don't set .need_free right: expect %d, got %d", func, need_free, s->need_free);
    cr_expect(s->bytes == bytes, "%s() don't set .bytes right: expect %d, got %d", func, bytes, s->bytes);
    cr_expect(s->chars == chars, "%s() don't set .chars right: expect %d, got %d", func, chars, s->chars);
    cr_expect(s->encoding == encoding, "%s() don't set .encoding right: expect %d, got %d", func, encoding, s->encoding);
    cr_expect(s->start == start, "%s() don't set .start right: expect %p, got %p", func, start, s->start);
    cr_expect(s->offset == offset, "%s() don't set .offset right: expect %d, got %d", func, offset, s->offset);
    cr_expect(get_entity(s) == ent, "%s() ain't refering to %p", func, ent);
    cr_expect(get_entity(s)->slcs == slcs, "%s() don't add references: expect %d, got %d", func, slcs, get_entity(s)->slcs);
} // check_slice

Test(Creation, nstr_refer_to_cstr)
{
    const char_t cstr[] = {"Hello world!"};
    int32_t size = sizeof(cstr) - 1;
    nstr_p new = NULL;

    new = nstr_refer_to_cstr(cstr, STR_ENC_ASCII);
    check_slice((const char_t *)"nstr_refer_to_cstr", new, 1, size, size, STR_ENC_ASCII, cstr, -1, &cstr_dummy, 1);

    nstr_delete(new);
} // nstr_new
