#pragma once

#include <common.h>

#define SERIAL_PORT 0x3F8

/// @brief Initialize the serial port.
/// @return True if successfully initialized the port, False, otherwise.
bool serial_init();

/// @brief Write a char to the serial port.
/// @param c Char to write.
void serial_write(const char c);