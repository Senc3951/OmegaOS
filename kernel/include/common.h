#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#define IRQ_SLAVE   2
#define IRQ0        0x20
#define IRQ15       0x2F

#define KERNEL_CS   0x8
#define KERNEL_DS   0x10

#define __PACKED__      __attribute__((packed))
#define __NO_RETURN__   __attribute__((noreturn))