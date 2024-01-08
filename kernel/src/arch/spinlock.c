#include <arch/spinlock.h>

extern void x64_acquire_lock(lock_t *lock);
extern void x64_release_lock(lock_t *lock);

void spinlock_acquire(lock_t *lock)
{
    x64_acquire_lock(lock);
}

void spinlock_release(lock_t *lock)
{
    x64_release_lock(lock);
}