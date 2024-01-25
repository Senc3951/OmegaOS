#pragma once

#include <common.h>

#define MAX_CORES   256
#define MAX_IO_APIC 8

typedef struct IO_APIC
{
    uint8_t id;
    uint8_t maxInterrupts;
    uint32_t address;
    uint64_t gsib;
} IOAPIC_t;

typedef struct MADT_INFO
{
    uint8_t coreIDs[MAX_CORES];
    IOAPIC_t ioapics[MAX_IO_APIC];
    uint64_t lapic;
    uint16_t coreCount, ioApicCount;
} MADTInfo_t;

/// @brief Get information from MADT.
void madt_init();

extern MADTInfo_t _MADT;