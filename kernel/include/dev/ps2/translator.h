#pragma once

#include <common.h>

#define NO_KEY          (-1)
#define LEFT_SHIFT      0x2A
#define RIGHT_SHIFT     0x36
#define CAPS_LOCK       0x3A
#define LEFT_SHIFT_REL  0xAA
#define RIGHT_SHFIT_REL 0xB6
#define INVALID_KEY     0xFF

/// @brief Translate a scancode to a character.
/// @param code Scancode of the character.
/// @param shift Is currently in shift mode.
/// @param caps Is currently in caps mode.
/// @return Character pressed.
char translate(const uint8_t code, const bool shift, const bool caps);