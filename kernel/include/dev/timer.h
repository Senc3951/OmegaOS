#pragma once

#include <common.h>

#define LAPIC_TIMER_ONE_SHOT    (0 << 17)
#define LAPIC_TIMER_PERIODIC    (1 << 17)
#define LAPIC_TIMER_MODE_MASK   (3 << 17)
#define LAPIC_TIMER_UNMASKED    (0 << 16)
#define LAPIC_TIMER_MASKED      (1 << 16)
#define LAPIC_TIMER_DIVIDER     3

/// @brief Initialize the Local APIC timer.
void lapic_timer_init();

/// @brief Configure the timer to fire an interrupt after certain time.
/// @param ms milliseconds that should pass before firing the interrupt.
void lapic_timer_oneshot(const uint64_t ms);

/// @brief Configure the timer to fire an interrupt after certain time.
/// @param ms milliseconds that should pass before firing the interrupt.
void lapic_timer_periodic(const uint64_t ms);