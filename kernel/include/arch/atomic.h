#pragma once

#include <common.h>

/// @brief Add amount to x atomically.
/// @param x Where to add the value to.
/// @param amount The amount to add.
/// @return Value before the increment.
uint64_t x64_atomic_add(uint64_t *x, uint64_t amount);