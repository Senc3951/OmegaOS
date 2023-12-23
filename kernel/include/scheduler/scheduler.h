#pragma once

#include <arch/isr.h>

void scheduler_init();

void yield(InterruptStack_t *stack);