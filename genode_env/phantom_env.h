#ifndef PHANTOM_ENV_H
#define PHANTOM_ENV_H

#include <base/component.h>
#include <base/heap.h>
#include "disk_backend.h"
#include "phantom_vmem.h"
#include "phantom_timer.h"
#include "phantom_threads.h"

namespace Phantom
{
    using namespace Genode;

    struct Main
    {
        Env &_env;
        Phantom::Vmem_adapter _vmem_adapter{_env};
        Heap _heap{_env.ram(), _env.rm()};
        Phantom::Timer_adapter _timer_adapter{_env};
        Phantom::PhantomThreadsRepo _threads_repo{_env, _heap};
        Phantom::Disk_backend _disk{_env, _heap};
        // XXX : docs says that it "allows only one timeout at a time"
        Timer::Connection _sleep_timer{_env, "phantom_sleep_timer"};

        Main(Env &env) : _env(env)
        {
        }
    };

    extern Main *main_obj;

    void test_obj_space(addr_t const addr_obj_space);
    void test_block_device(Disk_backend &disk);
}

#endif