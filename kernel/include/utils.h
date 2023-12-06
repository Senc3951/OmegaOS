#pragma once

#include <arch/isr.h>

/// @brief Dump the stack.
/// @param stack Stack to dump.
void dumpStack(InterruptStack_t *stack);