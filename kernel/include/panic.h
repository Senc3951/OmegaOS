#pragma once

#include <common.h>

#define panic(...)  __kpanic(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

/// @brief Kernel panic.
/// @param file File the panic occurred.
/// @param function Function the panic occurred.
/// @param line Line the panic occurred.
/// @param fmt Format of the string to print.
/// @attention This function will not return.
void __NO_RETURN__  __kpanic(const char *file, const char *function, const uint64_t line, const char *fmt, ...);