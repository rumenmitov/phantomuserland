#include <base/log.h>
#include "phantom_env.h"

extern "C"
{

#include <hal.h>
#include <unistd.h>

    void hal_sleep_msec(int miliseconds)
    {
        // Genode::log("Start to sleep (", miliseconds, "): ", main_obj->_sleep_timer.elapsed_ms());
        main_obj->_sleep_timer.msleep(miliseconds);
        // Genode::log("Finish to sleep (", miliseconds, "): ", main_obj->_sleep_timer.elapsed_ms());
    }

    void hal_disable_preemption()
    {
        // Genode::log("STUB: disabled preemption");
    }

    void hal_enable_preemption()
    {
        // Genode::log("STUB: enabled preemption");
    }
}