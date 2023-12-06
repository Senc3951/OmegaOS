#pragma once

#include <common.h>

#define assert(expr) {                                                          \
    if (!(expr))                                                                \
        __kassertation_failed(false, __FILE__, __FUNCTION__, __LINE__, #expr);  \
}

#define eassert(expr) {                                                         \
    if (!(expr))                                                                \
        __kassertation_failed(true, __FILE__, __FUNCTION__, __LINE__, #expr);   \
}

/// @brief Kernel assertation failed.
/// @param early Is the assert occurred before basic initializations.
/// @param file File the panic occurred.
/// @param function Function the panic occurred.
/// @param line Line the panic occurred.
/// @param expr Expression that failed.
/// @attention This function will not return.
void __NO_RETURN__  __kassertation_failed(const bool early, const char *file, const char *function, const uint64_t line, const char *expr);