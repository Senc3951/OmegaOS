#include <arch/lock.h>

extern void x64_acquire_lock(lock_t *lock);
extern void x64_release_lock(lock_t *lock);

void lock_acquire(lock_t *lock)
{
    x64_acquire_lock(lock);
}

void lock_release(lock_t *lock)
{
    x64_release_lock(lock);
}