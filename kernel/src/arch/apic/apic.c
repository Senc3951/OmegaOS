#include <arch/apic/apic.h>
#include <arch/apic/ipi.h>
#include <arch/apic/timer.h>
#include <arch/apic/madt.h>
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

bool _ApicInitialized = false;
uint32_t _BspID;

static void interruptHandler(InterruptStack_t *stack)
{
    LOG("Spurious interrupt - 0x%x (0x%x)\n", stack->interruptNumber, stack->errorCode);
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

void apic_set_registers()
{
    // Set base
    setBase(_MADT.lapic);
    
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
    
    // Send init ipi
    ipi_send_core(id, APIC_DELMOD_INIT, 0);
    if (!ipi_wait_accept())
        panic("[BSP] Failed to deliver INIT to core %u\n", id);
    
    // Send deassert ipi
    ipi_send_deassert(id, APIC_DELMOD_INIT, 0);
    if (!ipi_wait_accept())
        panic("[BSP] Failed to deassert core %u\n", id);
        
    lapic_timer_msleep(10);

    // Send startup ipi
    for (int j = 0; j < 2; j++)
    {
        apic_write_register(LAPIC_ESR, 0);  // Clear apic errors
        
        // Send sipi ipi
        ipi_send_core(id, APIC_DELMOD_START, AP_TRAMPOLINE >> 12);
        if (!ipi_wait_accept())
            panic("[BSP] Failed to deliver SIPI %d to core %u\n", j, id);
        
        lapic_timer_msleep(1);
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
    _CoreCount = _MADT.coreCount;
    uint64_t corePages = RNDUP(_CoreCount * sizeof(CoreContext_t), PAGE_SIZE);
    assert(_Cores = (CoreContext_t *)vmm_createIdentityPages(_KernelPML4, corePages, VMM_KERNEL_ATTRIBUTES));
    
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
    
    LOG("APIC at %p. Version: %u\n", _MADT.lapic, (apic_read_register(LAPIC_VER) >> 16) & 0xFF);
    _BspID = apic_read_register(LAPIC_ID);
    
    // Initialize apic registers & ipi
    assert(isr_registerHandler(SPURIOUS_ISR, interruptHandler));
    apic_set_registers();
    ipi_init();

    // Verify that APIC is enabled
    __rdmsr(IA32_APIC_BASE_MSR, &lo, &hi);
    assert(1 & (lo >> 11));
    
    _ApicInitialized = true;
}