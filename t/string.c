#include "mln_string.h"
#include <assert.h>

int main(void)
{
    mln_string_t text = mln_string("Hello");
    mln_string_t *slices, *ref, *s = mln_string_dup(&text);
    assert(s != NULL);

    assert((ref = mln_string_ref(s)) == s);

    assert(!mln_string_strcmp(ref, &text));

    assert((slices = mln_string_slice(ref, "e")) != NULL);

    assert(slices[0].len > 0);
    assert(!mln_string_const_strcmp(&slices[0], "H"));
    assert(slices[1].len > 0);
    assert(!mln_string_const_strcmp(&slices[1], "llo"));

    mln_string_slice_free(slices);
    mln_string_free(ref);
    mln_string_free(s);
    return 0;
}
