#pragma once

#include <common.h>

#define IOAPICID    0
#define IOAPICVER   1
#define IOAPICARB   2

#define IOREDTBL0   0x10
#define IOREDTBL1   0x12
#define IOREDTBL2   0x14

#define IOAPIC_DESTMODE_PHYSICAL    0
#define IOAPIC_DESTMODE_LOGICAL     1

#define IOAPIC_DELVMOD_FIXED        0
#define IOAPIC_DELVMOD_LOWEST       1
#define IOAPIC_DELVMOD_SMI          2
#define IOAPIC_DELVMOD_NMI          4
#define IOAPIC_DELVMOD_INIT         5
#define IOAPIC_DELVMOD_EXT_INT      7

#define IOAPIC_POLARITY_HIGH        0
#define IOAPIC_POLARITY_LOW         1

#define IOAPIC_TRIGGER_EDGE         0
#define IOAPIC_TRIGGER_LEVEL        1

#define IOAPIC_UNMASK               0
#define IOAPIC_MASK                 1

typedef union IOAPIC_REDIRECTION_ENTRY
{
    struct
    {
        uint64_t vector       : 8;
        uint64_t delvMode     : 3;
        uint64_t destMode     : 1;
        uint64_t delvStatus   : 1;
        uint64_t pinPolarity  : 1;
        uint64_t remoteIRR    : 1;
        uint64_t triggerMode  : 1;
        uint64_t mask         : 1;
        uint64_t reserved     : 39;
        uint64_t destination  : 8;
    };
    uint64_t raw;
} __PACKED__ IoApicRedirectionEntry_t;

/// @brief Initialize the IOAPIC.
void ioapic_init();

/// @brief Map an irq interrupt to an IO APIC one.
/// @param ioredtbl I/O APIC redtbl index.
/// @param vector Interrupt that will be raised.
/// @param logical Is destination mode logical.
void ioapic_map_irq(const uint8_t ioredtbl, const uint8_t vector, bool logical);