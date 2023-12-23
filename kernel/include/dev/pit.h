#pragma once

#include <common.h>

#define PIT_DEFAULT_FREQUENCY           100
#define PIT_COMMAND_PORT                0x43

#define PIT_CHANNEL_0                   0x40
#define PIT_CHANNEL_1                   0x41
#define PIT_CHANNEL_2                   0x42

#define PIT_BINARY                      0
#define PIT_BCD                         1

#define PIT_INTERRUPT_IN_TERMINAL       0
#define PIT_HARDWARE_TRIGGERABLE        0x2
#define PIT_RATE_GENERATOR              0x4
#define PIT_SQUARE_WAVE_GENERATOR       0x6
#define PIT_SOFTWARE_TRIGGERED_STROBE   0x8
#define PIT_HARDWARE_TRIGGERED_STROBE   0xA

#define PIT_LATCH_COUNT                 0
#define PIT_LOBYTE                      0xF
#define PIT_HIBYTE                      0x20
#define PIT_LOHIBYTE                    0x30

#define PIT_CHANNEL0                    0
#define PIT_CHANNEL1                    0x40
#define PIT_CHANNEL2                    0x80
#define PIT_CHANNELk                    0xC0

/// @brief Initialize the PIT.
/// @param frequency Frequency of the pit.
void pit_init(const uint16_t frequency);

/// @brief Sleep for a specified amount of milliseconds
/// @param milliseconds Milliseconds to sleep.
void sleep(const size_t milliseconds);