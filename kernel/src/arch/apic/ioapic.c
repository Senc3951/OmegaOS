#include <arch/apic/ioapic.h>
#include <arch/apic/madt.h>
#include <logger.h>

#define IOREGSEL    0
#define IOWIN       0x10
#define IOREDTBL    0x10

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

static void setEntry(uint64_t base, uint8_t index, uint64_t data)
{
    writeRegister(base, IOREDTBL + index * 2, (uint32_t)data);
    writeRegister(base, IOREDTBL + index * 2 + 1, (uint32_t)(data >> 32));
}

void ioapic_init()
{
    for (uint16_t i = 0; i < _MADT.ioApicCount; i++)
    {
        IOAPIC_t *ioapic = &_MADT.ioapics[i];
        ioapic->maxInterrupts = ((readRegister(ioapic->address, IOAPIC_REG_REDIRECTION_ENTRIES) >> 16) & 0xFF) + 1;
        for (uint8_t i = 0; i < ioapic->maxInterrupts; i++)
            setEntry(ioapic->address, i, i << 16);  // Disable interrupt
        
        LOG("[I/O APIC %u] Max Interrupts: %u. Address: %p. GSI Base: %p\n", ioapic->id, ioapic->maxInterrupts, ioapic->address, ioapic->gsib);
    }
}