#include <arch/apic/apic.h>
#include <arch/apic/madt.h>
#include <arch/cpu.h>
#include <assert.h>
#include <logger.h>

static void setBase(uint64_t base)
{
    const uint32_t IA32_APIC_BASE_MSR = 0x1B;
    const uint32_t IA32_APIC_BASE_MSR_ENABLE = 0x800;
    
    uint32_t edx = 0;
    uint32_t eax = (base & 0xfffff0000) | IA32_APIC_BASE_MSR_ENABLE;

#ifdef __PHYSICAL_MEMORY_EXTENSION__
    edx = (base >> 32) & 0x0f;
#endif

    __wrmsr(IA32_APIC_BASE_MSR, eax, edx);
}

uint32_t apic_read_register(const uint32_t offset)
{
    return *(volatile uint32_t *)(_MADT.lapic + offset);
}

void apic_write_register(const uint32_t offset, const uint32_t value)
{
    *(volatile uint32_t *)(_MADT.lapic + offset) = value;
}

void apic_send_eoi()
{
    apic_write_register(LAPIC_REG_EOI, 0);
}

void apic_init()
{
    uint32_t unused, edx;
    __cpuid(1, &unused, &unused, &unused, &edx);
    assert(edx & CPUID_FEAT_EDX_APIC);
    
    setBase(_MADT.lapic);
    apic_write_register(LAPIC_REG_SPURIOUS_IV, apic_read_register(LAPIC_REG_SPURIOUS_IV) | 0x100);
}