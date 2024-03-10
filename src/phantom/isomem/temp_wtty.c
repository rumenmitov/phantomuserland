#include <wtty.h>
#include "genode_misc.h"

errno_t wtty_putc_nowait(wtty_t *w, int c)
{
    _stub_print();
    return 0;
}

int wtty_getc(wtty_t *w)
{
    _stub_print();
    return 0;
}