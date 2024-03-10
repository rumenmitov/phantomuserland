
// #include <stdint.h>
// #include <machine/atomic.h>

#include <kernel/atomic.h>
#include "genode_misc.h"

int atomic_add(volatile int *val, int incr)
{
    // atomic_add_int(val, incr);
    *val += incr;
    return *val;
}

// int atomic_or(volatile int *val, int incr)
// {
//     atomic
// }