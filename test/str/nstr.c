#include <criterion/criterion.h>

#ifndef NSTR_SOURCE
#define NSTR_SOURCE 1
#include "str/nstr.c"
#endif

Test(Creation, nstr_new_blank)
{
    nstr_p blk1 = nstr_new_blank(STR_ENC_ASCII);
    nstr_p blk2 = nstr_new_blank(STR_ENC_UTF8);

    cr_expect(blk1 != NULL, "nstr_new_blank() return new pointer: expect non-NULL, got NULL");
    cr_expect(blk1 != &blanks[STR_ENC_ASCII], "nstr_new_blank() return pointer to constant blank");
    cr_expect(blk2 != &blanks[STR_ENC_UTF8], "nstr_new_blank() return pointer to constant blank");
    cr_expect(blk2 != blk1, "nstr_new_blank() return same pointer");
    cr_expect(blank_ent.slcs == 5, "nstr_new_blank() don't add references: expect %d, got %d", 2, blank_ent.slcs);
    cr_expect(blank_ent.need_free == 0);

    nstr_delete(blk2);
    nstr_delete(blk1);
} // nstr_new_blank

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

Test(Creation, nstr_new_reference)
{
    const char_t cstr[] = {"Hello world!"};
    int32_t size = sizeof(cstr) - 1;
    nstr_p new = NULL;

    new = nstr_new_reference(cstr, size);
    check_slice((const char_t *)"nstr_new_reference", new, 1, size, size, STR_ENC_ASCII, cstr, -1, &cstr_ent, 1);

    nstr_delete(new);
} // nstr_new
