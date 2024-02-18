#pragma once

#include <gui/printf.h>
#include <sys/scheduler.h>

#ifdef DEBUG
    #define LOG(...)    fctprintf(__kfctprintf, NULL, __VA_ARGS__)
#else
    #define LOG(...)    { }
#endif

#ifdef SYS_DEBUG
    #define LOG_PROC(...) ({                    \
        Process_t *curr = currentProcess();     \
        LOG("[%s/%u] ", curr->name, curr->id);  \
        LOG(__VA_ARGS__);                       \
    })
#else
    #define LOG_PROC(...) { }
#endif

/// @brief Print a char.
/// @param c Char to print.
/// @param arg Argument passed.
void __kfctprintf(char c, void *arg);