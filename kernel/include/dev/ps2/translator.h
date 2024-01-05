#pragma once

#include <common.h>

#define NO_KEY          ((uint8_t)-1)
#define CAPS_KEY        0x58
#define RELEASED_KEY    0xF0
#define RIGHT_SHIFT     0x12
#define LEFT_SHIFT      0x59
#define BACKSPACE       0x66
#define BACKTICK        0x0E
#define TAB             0x0D
#define INVALID_KEY     0xFF

/// @brief Translate a scancode to a character.
/// @param code Scancode of the character.
/// @param caps Is in caps mode.
/// @param shift Is in shift mode.
/// @return Character pressed.
uint8_t translate(const uint8_t code, const bool caps, const bool shift);