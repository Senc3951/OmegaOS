#pragma once

#include <gui/printf.h>
#include <sys/scheduler.h>

#ifdef DEBUG
    #define LOG(...)    fctprintf(__kfctprintf, NULL, __VA_ARGS__)
#else
    #define LOG(...)    { }
#endif

#ifdef SYS_DEBUG
    #define LOG_PROC(...) ({                                                    \
        LOG("[%s/%u] ", _CurrentProcess->name, _CurrentProcess->id);            \
        LOG(__VA_ARGS__);                                                       \
    })
#else
    #define LOG_PROC(proc, ...) { }
#endif

/// @brief Print a char.
/// @param c Char to print.
/// @param arg Argument passed.
void __kfctprintf(char c, void *arg);