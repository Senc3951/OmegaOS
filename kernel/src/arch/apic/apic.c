#include <arch/apic/apic.h>
#include <arch/apic/madt.h>
#include <arch/apic/timer.h>
#include <arch/cpu.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <panic.h>
#include <logger.h>

#define IA32_APIC_BASE_MSR      0x1B
#define APIC_SOFTWARE_ENABLE    (1 << 8)
#define CPUID_FEAT_EDX_APIC     (1 << 9)
#define CPUID_FEAT_ECX_X2APIC   (1 << 21)
#define IA32_APIC_MSR_ENABLE    (1 << 11)
#define LAPIC_NMI               (4 << 8)
#define APIC_ICR_STATUS         (1 << 12)

#define WAIT_FOR_DELIVERY() ({                                      \
    do {                                                            \
        __PAUSE();                                                  \
    } while (apic_read_register(LAPIC_ICRLO) & APIC_ICR_STATUS);    \
})

bool _ApicInitialized = false;
uint32_t _BspID;

static void interruptHandler(InterruptStack_t *stack)
{
    LOG("Spurious interrupt - 0x%x (0x%x)\n", stack->interruptNumber, stack->errorCode);
}

static void ipiInterruptHandler(InterruptStack_t *stack)
{
    UNUSED(stack);
    
    __CLI();
    LOG("Core have been requested to stop\n");
    __HCF();
}

static void setBase(uint64_t base)
{
    static const uint64_t MSR_APICBASE_ADDRMASK = 0x000ffffffffff000ULL;

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

void apic_eoi()
{
    apic_write_register(LAPIC_EOI, 0);
}

uint32_t apic_get_id()
{    
    return apic_read_register(LAPIC_ID) >> 24;
}

void apic_broadcast_ipi(IpiDeliveryMode_t mode, const uint8_t vector)
{
    InterruptCommandRegister_t icrlo = { 0 };
    icrlo.vector = vector;
    icrlo.delvMode = mode;
    icrlo.destType = APIC_DEST_SHORTHAND_ALL_BUT_SELF;
    icrlo.level = APIC_LEVEL_ASSERT;
    icrlo.trigger = APIC_TRIGGER_EDGE;
    uint32_t icrhi = 0;
    
    apic_write_register(LAPIC_ICRHI, icrhi);
    apic_write_register(LAPIC_ICRLO, icrlo.raw);
}

void apic_set_registers()
{
    // Initialize the APIC to a well known state
    apic_write_register(LAPIC_LDR, (apic_read_register(LAPIC_LDR) & 0xFFFFFF) | 1);
    apic_write_register(LAPIC_DFR, UINT32_MAX);
    apic_write_register(LAPIC_LINT0, APIC_LVT_RESET);
    apic_write_register(LAPIC_LINT1, APIC_LVT_RESET);
    apic_write_register(LAPIC_ESR, 0);
    apic_write_register(LAPIC_ESR, 0);
    apic_write_register(LAPIC_PERF, LAPIC_NMI);
    apic_write_register(LAPIC_SVR, SPURIOUS_ISR | APIC_SOFTWARE_ENABLE);
    apic_write_register(LAPIC_TPR, 0);
    apic_eoi();
    
    // Enable the APIC
    apicEnable();
}

void apic_wakeup_core(const uint8_t id)
{
    apic_write_register(LAPIC_ESR, 0);  // Clear apic errors

    // Select AP
    uint32_t v1 = (apic_read_register(LAPIC_ICRHI) & 0x00ffffff) | (id << 24);
    apic_write_register(LAPIC_ICRHI, v1);
    
    // Trigger INIT IPI
    apic_write_register(LAPIC_ICRLO, (apic_read_register(LAPIC_ICRLO) & 0xfff32000) | 0xc500);
    WAIT_FOR_DELIVERY();
    apic_write_register(LAPIC_ICRHI, v1);   // Select AP

    // DeAssert
    apic_write_register(LAPIC_ICRLO, (apic_read_register(LAPIC_ICRLO) & 0xfff00000) | 0x8500);
    
    WAIT_FOR_DELIVERY();
    lapic_timer_msleep(10);
    
    // Send STARTUP IPI
    for (int j = 0; j < 2; j++)
    {
        apic_write_register(LAPIC_ESR, 0);  // Clear apic errors

        // Select AP
        apic_write_register(LAPIC_ICRHI, v1);

        // Trigger STARTUP IPI
        uint32_t v4 = (apic_read_register(LAPIC_ICRLO) & 0xfff0f800) | 0x608;
        apic_write_register(LAPIC_ICRLO, v4);

        lapic_timer_msleep(1);
        WAIT_FOR_DELIVERY();
    }
}

CoreContext_t *currentCPU()
{
    uint32_t cid = apic_get_id();
    for (uint32_t i = 0; i < _CoreCount; i++)
    {
        if (cid == _Cores[i].id)
            return &_Cores[i];
    }
    
    panic("Failed finding core with id %u\n", cid);
}

void apic_init()
{
    // Verify APIC is supported
    uint32_t unused, ecx, edx, lo, hi;
    if (!__get_cpuid(1, &unused, &unused, &ecx, &edx))
    {
        LOG("[APIC] cpuid failed\n");
        return;
    }
    if (!(edx & CPUID_FEAT_EDX_APIC))
    {
        LOG("APIC not supported\n");
        return;
    }
    if (ecx & CPUID_FEAT_ECX_X2APIC)
    {
        LOG("x2APIC available but not supported\n");
    }
    
    // Set the APIC base
    setBase(_MADT.lapic);
    LOG("APIC at %p. Version: %u\n", _MADT.lapic, (apic_read_register(LAPIC_VER) >> 16) & 0xFF);
    
    _BspID = apic_read_register(LAPIC_ID);
    assert(isr_registerHandler(SPURIOUS_ISR, interruptHandler));
    assert(isr_registerHandler(IPI_ISR, ipiInterruptHandler));
    apic_set_registers();

    // Verify that APIC is enabled
    __rdmsr(IA32_APIC_BASE_MSR, &lo, &hi);
    assert(1 & (lo >> 11));
    
    _ApicInitialized = true;
}