#include "phantom_env.h"

extern "C"
{
#include <hal.h>

#include <kernel/timedcall.h>
#include <kernel/stats.h>
#include <kernel/board.h>

    void board_init_kernel_timer(void)
    {
        main_obj->_timer_adapter.set_handler(hal_time_tick);
    }

    static unsigned int msecDivider = 0;
    static unsigned int secDivider = 0;

    //! Called from timer interrupt, tick_rate is in uSec
    void hal_time_tick(int tick_rate_us)
    {
        msecDivider += tick_rate_us;

        while (msecDivider > 1000)
        {
            msecDivider -= 1000;
            phantom_process_timed_calls();

            secDivider++;
            if (secDivider > 1000)
            {
                secDivider -= 1000;
                // Once a sec
                stat_update_second_stats();
            }
        }
        // putchar('^');
    }
}