#include <base/mutex.h>
#include <base/semaphore.h>
#include <base/blockade.h>

extern "C"
{

    struct phantom_mutex_impl
    {
        Genode::Mutex *lock;
        const char *name;
    };

    struct phantom_sem_impl
    {
        Genode::Semaphore *sem;
        const char *name;
    };

    struct phantom_cond_impl
    {
        // Counter of threads waiting on blockade
        volatile int counter;
        // Mutex to ensure atomic access to counter
        Genode::Mutex *mutex;

        Genode::Blockade *blockade;
        const char *name;
    };
}