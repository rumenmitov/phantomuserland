#include <cpu/atomic.h>
#include <cpu/memory_barrier.h>

#include "phantom_sync.h"
#include "phantom_env.h"

// TODO : Error handling!
// TODO : Verify that snapshotted structures behave correctly

// XXX : Comments:

// XXX : - Sometimes, Phantom checks impl if it is 0, so assigning 0 to them on destroy
// XXX : - Still not sure if it is checked outside interface implementation

// XXX : - Do we need to unblock everything on the destruction of the structure?
// XXX : - Probably no, but should give warnings

extern "C"
{

#include <hal.h>

    /*
     *
     * Mutexes
     *
     */

    int hal_mutex_init(hal_mutex_t *m, const char *name)
    {
        main_obj->_heap.alloc(sizeof(Genode::Mutex), (void **)&m->impl->lock);
        construct_at<Genode::Mutex>(m->impl->lock);

        m->impl->name = name;

        return 0;
    }

    int hal_mutex_lock(hal_mutex_t *m)
    {
        m->impl->lock->acquire();
        return 0;
    }

    int hal_mutex_unlock(hal_mutex_t *m)
    {
        m->impl->lock->release();
        return 0;
    }

    int hal_mutex_is_locked(hal_mutex_t *m)
    {
        (void)m;
        // TODO : Implement using modified Mutexes!
        Genode::warning("STUB: Checked if mutex locked. Returned false");
        return 0;
    }

    errno_t hal_mutex_destroy(hal_mutex_t *m)
    {
        m->impl->lock->~Mutex();
        main_obj->_heap.free(m->impl->lock, sizeof(Genode::Mutex));
        m->impl->lock = 0;
        return 0;
    }

    /*
     *
     * Semaphores
     *
     */

    int hal_sem_acquire(hal_sem_t *s)
    {
        s->impl->sem->up();
        return 0;
    }

    void hal_sem_release(hal_sem_t *s)
    {
        s->impl->sem->down();
    }

    int hal_sem_init(hal_sem_t *s, const char *name)
    {
        main_obj->_heap.alloc(sizeof(Genode::Semaphore), (void **)&s->impl->sem);
        construct_at<Genode::Semaphore>(s->impl->sem);

        s->impl->name = name;

        return 0;
    }

    void hal_sem_destroy(hal_sem_t *s)
    {
        s->impl->sem->~Semaphore();
        main_obj->_heap.free(s->impl->sem, sizeof(Genode::Semaphore));
        s->impl->sem = 0;
    }

    /*
     *
     * Spinlocks
     *
     */

    void hal_spin_init(hal_spinlock_t *sl)
    {
        sl->lock = 0;
    }

    void hal_spin_lock(hal_spinlock_t *sl)
    {
        // Just spinning, however in Genode's implementation we also use thread.yield
        while (!Genode::cmpxchg(&sl->lock, 0, 1))
            ;
    }

    void hal_spin_unlock(hal_spinlock_t *sl)
    {
        Genode::memory_barrier();
        sl->lock = 0;
    }

    /*
     *
     * Conditional variables
     *
     */

    int hal_cond_init(hal_cond_t *c, const char *name)
    {
        main_obj->_heap.alloc(sizeof(Genode::Blockade), (void **)&c->impl->blockade);
        construct_at<Genode::Blockade>(c->impl->blockade);
        main_obj->_heap.alloc(sizeof(Genode::Mutex), (void **)&c->impl->mutex);
        construct_at<Genode::Mutex>(c->impl->mutex);

        c->impl->name = name;

        return 0;
    }

    errno_t hal_cond_wait(hal_cond_t *c, hal_mutex_t *m)
    {
        assert(c->impl);
        // Updating counter first
        c->impl->mutex->acquire();
        c->impl->counter++;
        c->impl->mutex->release();

        // XXX : Not really a good time to preempt the thread. It can be stuck here forever

        // Waiting
        hal_mutex_unlock(m);
        c->impl->blockade->block();
        hal_mutex_lock(m);
        return 0;
    }

    // XXX : Seems that it is almost not used. Hence we can ignore for now
    errno_t hal_cond_timedwait(hal_cond_t *c, hal_mutex_t *m, long msecTimeout)
    {
        (void)c;
        (void)m;
        (void)msecTimeout;
        Genode::warning("STUB: Attempted to hal_cond_timewait()");
        return 1;
    }

    // XXX : Wake up only one thread waiting on a cond.
    // Not used a lot. dpc_timed() in isomem, deffered_refdec() in vm
    // There are more, but, probably, functinos containing them are not used
    errno_t hal_cond_signal(hal_cond_t *c)
    {
        // Updating counter first
        c->impl->mutex->acquire();
        if (c->impl->counter > 0)
            c->impl->counter--;
        // Waking up the thread
        // Nothing bad if we will wakeup one more time
        c->impl->blockade->wakeup();
        c->impl->mutex->release();
        return 0;
    }

    // Wake up all threads waiting on the cond
    errno_t hal_cond_broadcast(hal_cond_t *c)
    {
        assert(c->impl);
        c->impl->mutex->acquire();
        while (c->impl->counter > 0)
        {
            c->impl->counter--;
            c->impl->blockade->wakeup();
        }
        c->impl->mutex->release();
        return 0;
    }

    errno_t hal_cond_destroy(hal_cond_t *c)
    {

        if (c->impl->counter > 0)
        {
            Genode::warning("Trying to destroy cond with waiting threads (counter=%d)", c->impl->counter);
        }

        c->impl->blockade->~Blockade();
        main_obj->_heap.free(c->impl->blockade, sizeof(Genode::Blockade));

        c->impl->mutex->~Mutex();
        main_obj->_heap.free(c->impl->blockade, sizeof(Genode::Mutex));

        c->impl = 0;
        return 0;
    }
}