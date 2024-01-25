#include <arch/apic/apic.h>
#include <arch/apic/madt.h>
#include <arch/cpu.h>
#include <assert.h>
#include <logger.h>

#define IA32_APIC_BASE_MSR  0x1B
#define CPUID_FEAT_EDX_APIC 512

static void setBase(uint64_t base)
{
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

uint32_t apic_get_id()
{
    return apic_read_register(LAPIC_ID_REGISTER) >> 24;
}

void apic_init()
{
    uint32_t unused, edx;
    assert(__get_cpuid(1, &unused, &unused, &unused, &edx) && (edx & CPUID_FEAT_EDX_APIC));
        
    // Hardware enable the APIC
    LOG("Local APIC at %p\n", _MADT.lapic);
    setBase(_MADT.lapic);
    
    // Clear the task priority register
    apic_write_register(LAPIC_TASK_PRIORITY_REGISTER, 0);
        
    // Enable spurious interrupt vector
    apic_write_register(LAPIC_REG_SPURIOUS_IV, apic_read_register(LAPIC_REG_SPURIOUS_IV) | 0x100);
}