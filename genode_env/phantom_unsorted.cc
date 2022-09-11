#include <base/log.h>

extern "C"
{

#include <hal.h>
#include <unistd.h>

    void hal_sleep_msec(int miliseconds)
    {

        int res = -1;
        struct timespec ts;

        ts.tv_sec = miliseconds / 1000;
        ts.tv_nsec = (miliseconds % 1000) * 1000000;

        do
        {
            res = nanosleep(&ts, &ts);
        } while (res && errno == EINTR);
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