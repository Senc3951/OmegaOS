#pragma once

#include <common.h>

typedef volatile uint32_t lock_t;

#define MAKE_SPINLOCK(name) static __ALIGNED__(sizeof(lock_t)) lock_t name = 0

/// @brief Acquire a lock.
/// @param lock lock to acquire.
void lock_acquire(lock_t *lock);

/// @brief Release a lock.
/// @param lock Lock to release.
void lock_release(lock_t *lock);

/// @brief Check if a lock is being used.
/// @param lock Lock to check.
/// @return 1 if used, 0, otherwise.
int lock_used(lock_t *lock);