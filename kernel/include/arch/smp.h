#pragma once

#include <arch/cpu.h>

typedef struct LOCAL_APIC_PROCESSOR
{
    uint8_t apStatus;
    uint8_t bspStatus;
    uint32_t pml4;
    uint64_t stackTop;
    uint64_t apEntry;
    uint64_t context;
} __PACKED__ LocalApicProcessor_t;

/// @brief Initialize other cores.
void smp_init();