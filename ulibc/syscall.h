#pragma once

#include <stdint.h>
#include <stddef.h>

typedef long ssize_t;

#define SYSCALL_READ        0
#define SYSCALL_WRITE       1
#define SYSCALL_OPEN        2
#define SYSCALL_CLOSE       3
#define SYSCALL_EXIT        4
#define SYSCALL_MKDIR       5
#define SYSCALL_FTELL       6
#define SYSCALL_LSEEK       7
#define SYSCALL_OPENDIR     8
#define SYSCALL_READDIR     9
#define SYSCALL_CLOSEDIR    10
#define SYSCALL_CHDIR       11
#define SYSCALL_GETCWD      12

#define SYSCALL_0(n) ({             \
    uint64_t __result;              \
    asm volatile(                   \
        "int $0x80"                 \
        : "=a" (__result)           \
        : "0" (n)                   \
        : "rcx", "r11", "memory"    \
    );                              \
    __result;                       \
})

#define SYSCALL_1(n, arg1) ({       \
    uint64_t __result;              \
    asm volatile(                   \
        "int $0x80"                 \
        : "=a" (__result)           \
        : "0" (n), "D" (arg1)       \
        : "rcx", "r11", "memory"    \
    );                              \
    __result;                       \
})

#define SYSCALL_2(n, arg1, arg2) ({         \
    uint64_t __result;                      \
    asm volatile(                           \
        "int $0x80"                         \
        : "=a" (__result)                   \
        : "0" (n), "D" (arg1), "S" (arg2)   \
        : "rcx", "r11", "memory"            \
    );                                      \
    __result;                               \
})

#define SYSCALL_3(n, arg1, arg2, arg3) ({               \
    uint64_t __result;                                  \
    asm volatile(                                       \
        "int $0x80"                                     \
        : "=a" (__result)                               \
        : "0" (n), "D" (arg1), "S" (arg2), "d" (arg3)   \
        : "rcx", "r11", "memory"                        \
    );                                                  \
    __result;                                           \
})

#define SYSCALL_4(n, arg1, arg2, arg3, arg4) ({                     \
    uint64_t __result;                                              \
    asm volatile(                                                   \
        "int $0x80"                                                 \
        : "=a" (__result)                                           \
        : "0" (n), "D" (arg1), "S" (arg2), "d" (arg3), "r"(arg4)    \
        : "rcx", "r11", "memory"                                    \
    );                                                              \
    __result;                                                       \
})
