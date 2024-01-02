#include <arch/apic/ioapic.h>
#include <arch/apic/madt.h>

static uint32_t readRegister(uint64_t ioapicAddr, const uint8_t offset)
{
    *(volatile uint32_t *)(ioapicAddr) = offset;
    return *(volatile uint32_t *)(ioapicAddr + 0x10);
}

static void writeRegister(uint64_t ioapicAddr, const uint8_t offset, const uint32_t value)
{
    *(volatile uint32_t *)(ioapicAddr) = offset;
    *(volatile uint32_t *)(ioapicAddr + 0x10) = value;
}

void ioapic_init()
{
    IOAPIC_t *ioapic = &_MADT.ioapics[0];
    ioapic->maxInterrupts = ((readRegister(ioapic->address, IOAPIC_REG_REDIRECTION_ENTRIES) >> 16) & 0xFF) + 1;
}