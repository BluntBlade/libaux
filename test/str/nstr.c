#include <criterion/criterion.h>

#ifndef NSTR_SOURCE
#define NSTR_SOURCE 1
#include "str/nstr.c"
#endif

Test(Function, nstr_blank_string)
{
    nstr_p blk = nstr_blank();

    cr_expect(blk != NULL, "nstr_blank() return pointer: expect non-NULL, got NULL");
    cr_expect(blank_dummy.slcs == 1, "nstr_blank_string() don't add references: expect %d, got %d", 1, blank_dummy.slcs);

    nstr_delete(blk);
} // nstr_blank_string
