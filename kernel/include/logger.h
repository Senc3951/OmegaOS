#pragma once

#include <gui/printf.h>

#ifdef DEBUG
    #define LOG(...) fctprintf(__kfctprintf, NULL, __VA_ARGS__)
#else
    #define LOG(...) { }
#endif

/// @brief Print a char.
/// @param c Char to print.
/// @param arg Argument passed.
void __kfctprintf(char c, void *arg);