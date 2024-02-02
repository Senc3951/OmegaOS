#include <arch/apic/ioapic.h>
#include <arch/apic/apic.h>
#include <arch/apic/madt.h>
#include <assert.h>
#include <logger.h>

#define IOREGSEL    0
#define IOWIN       0x10

static uint32_t readRegister(uint64_t base, const uint8_t offset)
{
    *(volatile uint32_t *)(base + IOREGSEL) = offset;
    return *(volatile uint32_t *)(base + IOWIN);
}

static void writeRegister(uint64_t base, const uint8_t offset, const uint32_t value)
{
    *(volatile uint32_t *)(base + IOREGSEL) = offset;
    *(volatile uint32_t *)(base + IOWIN) = value;
}

static void writeRedirectionEntry(uint64_t base, uint8_t index, IoApicRedirectionEntry_t *entry)
{    
    writeRegister(base, IOREDTBL0 + index * 2, (uint32_t)entry->raw);
    writeRegister(base, IOREDTBL0 + index * 2 + 1, (uint32_t)(entry->raw >> 32));
}

static void maskInterrupt(uint64_t base, uint8_t index)
{
    IoApicRedirectionEntry_t entry;
    entry.raw = readRegister(base, IOREDTBL0 + index * 2);
    entry.mask = IOAPIC_MASK;
    writeRegister(base, IOREDTBL0 + index * 2, (uint32_t)entry.raw);
    
    uint32_t new = readRegister(base, IOREDTBL0 + index * 2);
    assert(new == (uint32_t)entry.raw);
}

static IOAPIC_t *getIoApicByInterrupt(const uint8_t irq)
{
    for (uint16_t i = 0; i < _MADT.ioapicCount; i++)
    {
        IOAPIC_t *ioapic = _MADT.ioapics[i];
        if (irq >= ioapic->gsib && irq <= ioapic->gsib + ioapic->maxInterrupts)
            return ioapic;
    }
    
    return NULL;
}

void ioapic_init()
{
    if (!_ApicInitialized)
        return;
    
    for (uint16_t i = 0; i < _MADT.ioapicCount; i++)
    {
        IOAPIC_t *ioapic = _MADT.ioapics[i];
        
        // Verify ID
        uint32_t id = (readRegister(ioapic->address, IOAPICID) >> 24) & 0xF0;
        assert(ioapic->id == id);
        
        // Get max interrupts and version
        uint32_t iovred = readRegister(ioapic->address, IOAPICVER);
        uint32_t version = iovred & 0x7F;
        ioapic->maxInterrupts = ((iovred >> 16) & 0xFF) + 1;
        
        // Mask all interrupts
        for (uint16_t j = 0; j < ioapic->maxInterrupts; j++)
            maskInterrupt(ioapic->address, j);
        
        LOG("[I/O APIC %u] Version: %u. Address: %p. GSI Base: %p. Max Interrupts: %u\n", ioapic->id, version, ioapic->address, ioapic->gsib, ioapic->maxInterrupts);
    }
    for (uint16_t i = 0; i < _MADT.ioapicInterruptOverrideCount; i++)
    {
        IoApicInterruptOverride_t *entry = _MADT.ioapicInterruptOverride[i];
        LOG("I/O APIC Interrupt Override. Bus source: %u. Irq Souce: %u. GSI: %p. Flags: %u\n", entry->busSource, entry->irqSource, entry->gsi, entry->flags);
    }
}

void ioapic_map_irq(const uint8_t ioredtbl, const uint8_t vector, bool logical)
{
    IoApicRedirectionEntry_t entry = { .raw = 0 };
    entry.vector = vector;
    entry.delvMode = IOAPIC_DELVMOD_FIXED;
    entry.pinPolarity = IOAPIC_POLARITY_HIGH;
    entry.triggerMode = IOAPIC_TRIGGER_EDGE;
    entry.mask = IOAPIC_UNMASK;
    
    if (logical)
    {
        entry.destMode = IOAPIC_DESTMODE_LOGICAL;
        entry.destination = UINT8_MAX;
    }
    else
    {
        entry.destMode = IOAPIC_DESTMODE_PHYSICAL;
        entry.destination = _BspID;
    }
    
    IOAPIC_t *ioapic = getIoApicByInterrupt(ioredtbl);
    assert(ioapic);
    
    LOG("[I/O APIC %u] mapping irq %u to vector %u with %s mode (0x%x)\n", ioapic->id, ioredtbl, vector, logical ? "logical" : "physical", entry.raw);
    writeRedirectionEntry(ioapic->address, ioredtbl, &entry);
}