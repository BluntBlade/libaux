#include <criterion/criterion.h>

#ifndef NSTR_SOURCE
#define NSTR_SOURCE 1
#include "str/nstr.c"
#endif

Test(Function, nstr_blank)
{
    nstr_p blk = nstr_blank();

    cr_expect(blk = &blank, "nstr_blank() return pointer: expect %p, got %p", &blank, blk);
    cr_expect(blank_dummy.slcs == 1, "nstr_blank() don't add references: expect %d, got %d", 1, blank_dummy.slcs);
    cr_expect(blank_dummy.need_free == 0);
    cr_expect(blank.need_free == 0);

    nstr_delete(blk);
} // nstr_blank
