#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#define _KB 1024ULL
#define _MB (_KB * 1024)
#define _GB (_MB * 1024)

#define KERNEL_START    (2 * _MB)

#define IRQ_SLAVE       2
#define IRQ0            0x20
#define IRQ15           0x2F

#define KERNEL_CS       0x8
#define KERNEL_DS       0x10

#define PAGE_SIZE       4096

#define RNDUP(num, nm)  (num < nm ? nm : ((num / nm) * nm))
#define RNDWN(num, nm)  (((num + nm - 1) / nm) * nm)

#define __PACKED__      __attribute__((packed))
#define __NO_RETURN__   __attribute__((noreturn))