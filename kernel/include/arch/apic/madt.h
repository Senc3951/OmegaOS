#pragma once

#include <common.h>

#define MAX_CORES               256
#define MAX_IO_APIC             8
#define MAX_IO_APIC_REDIRECTS   16

typedef struct IO_APIC
{
    uint8_t id;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsib;
    uint16_t maxInterrupts; // Overflow
} __PACKED__ IOAPIC_t;

typedef struct IO_APIC_INTERRUPT_OVERRIDE
{
    uint8_t busSource;
    uint8_t irqSource;
    uint32_t gsi;
    uint16_t flags;
} __PACKED__ IoApicInterruptOverride_t;

typedef struct MADT_INFO
{
    uint8_t coreIDs[MAX_CORES];
    IOAPIC_t *ioapics[MAX_IO_APIC];
    IoApicInterruptOverride_t *ioapicInterruptOverride[MAX_IO_APIC_REDIRECTS];
    uint64_t lapic;
    uint16_t coreCount, ioapicCount, ioapicInterruptOverrideCount;
} MADTInfo_t;

/// @brief Get information from MADT.
void madt_init();

extern MADTInfo_t _MADT;