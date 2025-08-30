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

Test(Creation, nstr_refer_to_cstr)
{
    const char_t cstr[] = {"Hello world!"};
    int32_t size = sizeof(cstr) - 1;
    nstr_p new = NULL;

    new = nstr_refer_to_cstr(cstr, STR_ENC_ASCII);
    cr_expect(new != NULL, "nstr_refer_to_cstr() return pointer: expect non-NULL, got NULL");
    cr_expect(new->need_free == 1, "nstr_refer_to_cstr() don't set .need_free right: expect %d, got %d", 1, new->need_free);
    cr_expect(new->bytes == size, "nstr_refer_to_cstr() don't set .bytes right: expect %d, got %d", size, new->bytes);
    cr_expect(new->chars == size, "nstr_refer_to_cstr() don't set .chars right: expect %d, got %d", size, new->chars);
    cr_expect(new->encoding == STR_ENC_ASCII, "nstr_refer_to_cstr() don't set .encoding right: expect %d, got %d", STR_ENC_ASCII, new->encoding);
    cr_expect(new->start == cstr, "nstr_refer_to_cstr() don't set .start right: expect %p, got %p", cstr, new->start);
    cr_expect(new->offset == -1, "nstr_refer_to_cstr() don't set .offset right: expect %d, got %d", -1, new->offset);
    cr_expect(get_entity(new) == &cstr_dummy, "nstr_refer_to_cstr() ain't refering to cstr_dummy");
    cr_expect(get_entity(new)->slcs == 1, "nstr_refer_to_cstr() don't add references: expect %d, got %d", 1, get_entity(new)->slcs);

    nstr_delete(new);
} // nstr_new
