#pragma once

/// @brief Initialize the ps2 keyboard.
void ps2_kbd_init();

/// @brief Read a character from the ps2 keyboard.
/// @return Character that was read.
char ps2_kbd_getc();