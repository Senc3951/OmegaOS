#pragma once

#include <gui/printf.h>

#define LOG(...) fctprintf(__kfctprintf, NULL, __VA_ARGS__)

/// @brief Print a char.
/// @param c Char to print.
/// @param arg Argument passed.
void __kfctprintf(char c, void *arg);