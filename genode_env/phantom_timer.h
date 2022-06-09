
#include <base/env.h>
#include <base/mutex.h>
#include <timer_session/connection.h>

namespace Phantom
{
    struct Timer_adapter;
}

struct Phantom::Timer_adapter
{
    Genode::Env &_env;
    Timer::Connection _timer{_env};
    const unsigned int rate_us = 1000;

    Timer::Periodic_timeout<Phantom::Timer_adapter> _periodic_timeout{
        _timer,
        *this,
        &Phantom::Timer_adapter::handle_timer_tick,
        Genode::Microseconds(rate_us)};

    Genode::Mutex _handler_mutex{};

    void (*_handler)(long long unsigned tick_rate) = nullptr;

    void handle_timer_tick(Genode::Duration duration)
    {
        Genode::Mutex::Guard guard(_handler_mutex);

        if (_handler != nullptr)
            _handler(duration.trunc_to_plain_us().value);
    }

    void set_handler(void (*handler)(long long unsigned tick_rate))
    {
        Genode::Mutex::Guard guard(_handler_mutex);

        _handler = handler;
    }

    Timer_adapter(Genode::Env &env) : _env(env)
    {
    }
};
