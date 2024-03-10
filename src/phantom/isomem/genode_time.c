#include <hal.h>
#include "genode_misc.h"

int hal_time_init()
{
    _stub_print();
    return 0;
}

bigtime_t hal_system_time(void)
{
    _stub_print();
    return 0;
}

bigtime_t hal_system_time_lores(void)
{
    _stub_print();
    return 0;
}

bigtime_t hal_local_time(void)
{
    _stub_print();
    return 0;
}