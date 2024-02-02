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

/// @brief Start the Local APIC timer.
/// @param isr Interrupt to fire.
void lapic_timer_start(const uint8_t isr);

/// @brief Configure the timer to fire an interrupt after certain time.
/// @param isr Interrupt to fire.
/// @param ms milliseconds that should pass before firing the interrupt.
void lapic_timer_oneshot(const uint8_t isr, const uint64_t ms);

/// @brief Configure the timer to fire an interrupt after certain time.
/// @param isr Interrupt to fire.
/// @param ms milliseconds that should pass before firing the interrupt.
void lapic_timer_periodic(const uint8_t isr, const uint64_t ms);

/// @brief Sleep for specific milliseconds.
/// @param ms ms to sleep.
void lapic_timer_msleep(const uint64_t ms);