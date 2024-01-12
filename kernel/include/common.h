#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>

typedef long ssize_t;

extern uint64_t _KernelStart, _KernelEnd, _KernelWritableStart, _KernelWritableEnd;

#define _KB                     1024ULL
#define _MB                     (_KB * 1024)
#define _GB                     (_MB * 1024)

#define ROOT_PID                1

#define IRQ_SLAVE               2
#define IRQ0                    0x20
#define IRQ15                   0x2F
#define SYSCALL                 0x80

#define PAGE_SIZE               4096ULL

#define KERNEL_STACK_SIZE       (8 * PAGE_SIZE)
#define USER_STACK_START        0x7FFFFFFFE000
#define USER_STACK_SIZE         (8 * PAGE_SIZE)

#define RNDUP(num, nm)          ((num) < nm ? nm : (((num) / nm) * nm))
#define RNDWN(num, nm)          ((((num) + nm - 1) / nm) * nm)
#define MAX(n1, n2)             (n1 < n2 ? n2 : n1)
#define UNUSED(x)               (void)(x)

#define __PACKED__      __attribute__((packed))
#define __NO_RETURN__   __attribute__((noreturn))
#define __PURE__        __attribute__((pure))
#define __MALLOC__      __attribute__((malloc))
#define __ALIGNED__(n)  __attribute__((aligned(n)))