#include <arch/apic/apic.h>
#include <arch/apic/madt.h>
#include <arch/cpu.h>
#include <arch/isr.h>
#include <assert.h>
#include <logger.h>

#define IA32_APIC_BASE_MSR      0x1B
#define APIC_SOFTWARE_ENABLE    (1 << 8)
#define CPUID_FEAT_EDX_APIC     (1 << 9)
#define IA32_APIC_MSR_ENABLE    (1 << 11)
#define LAPIC_NMI               (4 << 8)
#define MSR_APICBASE_ADDRMASK   0x000ffffffffff000ULL

bool _ApicInitialized = false;

static void interruptHandler(InterruptStack_t *stack)
{
    LOG("Spurious interrupt - 0x%x (0x%x)\n", stack->interruptNumber, stack->errorCode);
}

static void setBase(uint64_t base)
{
    uint32_t low, high;
    __rdmsr(IA32_APIC_BASE_MSR, &low, &high);
    uint64_t msr = ((uint64_t)high << 32) | low;

    msr &= ~MSR_APICBASE_ADDRMASK;
    base &= MSR_APICBASE_ADDRMASK;
    msr |= base;

    __wrmsr(IA32_APIC_BASE_MSR, msr, msr >> 32);
}

static void apicEnable()
{
    uint32_t low, high;
    __rdmsr(IA32_APIC_BASE_MSR, &low, &high);
    
    uint64_t msr = ((uint64_t)high << 32) | low;
    msr |= IA32_APIC_MSR_ENABLE;
    __wrmsr(IA32_APIC_BASE_MSR, msr, msr >> 32);
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
    apic_write_register(LAPIC_EOI, 0);
}

uint32_t apic_get_id()
{
    return apic_read_register(LAPIC_ID) >> 24;
}

void apic_disable()
{
    uint32_t low, high;
    __rdmsr(IA32_APIC_BASE_MSR, &low, &high);
    
    uint64_t msr = ((uint64_t)high << 32) | low;
    msr &= ~IA32_APIC_MSR_ENABLE;
    __wrmsr(IA32_APIC_BASE_MSR, msr, msr >> 32);
}

void apic_init()
{
    // Verify APIC is supported
    uint32_t unused, edx, lo, hi;
    if (!(__get_cpuid(1, &unused, &unused, &unused, &edx) && (edx & CPUID_FEAT_EDX_APIC)))
    {
        LOG("APIC not available\n");
        return;
    }
    
    // Set the APIC base
    setBase(_MADT.lapic);
    LOG("APIC at %p. Version: %u\n", _MADT.lapic, apic_read_register(LAPIC_VER));
    
    // Initialize the APIC to a well known state
    assert(isr_registerHandler(SPURIOUS_ISR, interruptHandler));
    apic_write_register(LAPIC_TPR, 0);
    apic_write_register(LAPIC_LDR, (apic_read_register(LAPIC_LDR) & 0xFFFFFF) | 1);
    apic_write_register(LAPIC_DFR, UINT32_MAX);
    apic_write_register(LAPIC_PERF, LAPIC_NMI);
    apic_write_register(LAPIC_LINT0, APIC_LVT_RESET);
    apic_write_register(LAPIC_LINT1, APIC_LVT_RESET);
    apic_write_register(LAPIC_SVR, SPURIOUS_ISR | APIC_SOFTWARE_ENABLE);
    apic_send_eoi();
    
    // Enable the APIC
    apicEnable();

    // Verify that APIC is enabled
    __rdmsr(IA32_APIC_BASE_MSR, &lo, &hi);
    assert(1 & (lo >> 11));
    
    _ApicInitialized = true;
}