#pragma once

#include <common.h>

typedef volatile uint32_t lock_t;

#define MAKE_SPINLOCK(name) static __ALIGNED__(sizeof(lock_t)) lock_t name = 0

/// @brief Acquire a lock.
/// @param lock lock to acquire.
void spinlock_acquire(lock_t *lock);

/// @brief Release a lock.
/// @param lock Lock to release.
void spinlock_release(lock_t *lock);