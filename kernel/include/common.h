#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

extern uint64_t _KernelStart, _KernelEnd;

#define _KB             1024ULL
#define _MB             (_KB * 1024)
#define _GB             (_MB * 1024)

#define IRQ_SLAVE       2
#define IRQ0            0x20
#define IRQ15           0x2F

#define KERNEL_CS       0x8
#define KERNEL_DS       0x10

#define PAGE_SIZE       4096

#define KRN_HEAP_START  (_KernelEnd + 4 * _MB)
#define KRN_HEAP_SIZE   (2 * _MB)

#define RNDUP(num, nm)  (num < nm ? nm : (((num) / nm) * nm))
#define RNDWN(num, nm)  (((num + nm - 1) / nm) * nm)
#define MAX(n1, n2)     (n1 < n2 ? n2 : n1)
#define UNUSED(x)       (void)(x)

#define __PACKED__      __attribute__((packed))
#define __NO_RETURN__   __attribute__((noreturn))
#define __ALIGNED__(n)  __attribute__((aligned(n)))