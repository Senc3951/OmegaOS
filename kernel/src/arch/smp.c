#include <arch/smp.h>
#include <arch/apic/madt.h>
#include <arch/apic/apic.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <dev/pit.h>
#include <libc/string.h>
#include <assert.h>
#include <panic.h>
#include <logger.h>

#define AP_TIMEOUT_MS       5
#define AP_TIMEOUT_RETRY    10

#define WAIT_FOR_DELIVERY() ({                                                  \
    do {                                                                        \
        asm volatile("pause" : : : "memory");                                   \
    } while (apic_read_register(LAPIC_REG_INTERRUPT_COMMAND0) & (1 << 12));     \
})

#define WAIT_FOR_CORE(i, core) ({                               \
    for (int j = 0; j < AP_TIMEOUT_RETRY; j++) {                \
        if (core->apStatus == 1)                                \
            break;                                              \
        \
        asm volatile("pause" : : : "memory");                   \
        sleep(AP_TIMEOUT_MS);                                   \
    }                                                           \
    \
    if (core->apStatus != 1)                                    \
        panic("Failed initializing core %u", _MADT.coreIDs[i]); \
})

CoreContext_t *_CoreContexts = NULL;

void smp_init()
{
    assert(_CoreContexts = pmm_getFrame());
    
    uint8_t bspid;
    asm volatile("mov $1, %%rax; cpuid; shrl $24, %%ebx;" : "=b"(bspid) : :);

    extern int ap_entry(uint64_t lapicID);
    
    // Copy the AP trampoline code to a fixed address
    extern void trampoline_code();
    extern void trampoline_data();
    void *trampolineCodePtr = (void *)&trampoline_code;
    void *trampolineDataPtr = (void *)&trampoline_data;
    
    vmm_identityMapPage(_KernelPML4, (void *)AP_TRAMPOLINE, VMM_KERNEL_ATTRIBUTES);
    memcpy((void *)AP_TRAMPOLINE, trampolineCodePtr, PAGE_SIZE);
    LocalApicProcessor_t *apicCore = (LocalApicProcessor_t *)(AP_TRAMPOLINE + ((uint64_t)trampolineDataPtr - (uint64_t)trampolineCodePtr));
    
    // Initialize each core
    for (uint16_t i = 0; i < _MADT.coreCount; i++)
    {
        uint8_t currentLapicID = _MADT.coreIDs[i];
        CoreContext_t *currentContext = (CoreContext_t *)((uint64_t)_CoreContexts + i * sizeof(CoreContext_t));
        
        apicCore->apStatus = apicCore->bspStatus = 0;
        if (currentLapicID == bspid)  // Skip initializing bspid (current)
            continue;
        
        // Send INIT IPI
        apic_write_register(LAPIC_ERR_STATUS_REGISTER, 0);  // Clear APIC errors

        // Select AP
        uint32_t v1 = (apic_read_register(LAPIC_REG_INTERRUPT_COMMAND1) & 0x00ffffff) | (i << 24);
        apic_write_register(LAPIC_REG_INTERRUPT_COMMAND1, v1);
        
        // Trigger INIT IPI
        uint32_t v2 = (apic_read_register(LAPIC_REG_INTERRUPT_COMMAND0) & 0xfff32000) | 0xc500;
        apic_write_register(LAPIC_REG_INTERRUPT_COMMAND0, v2);
    
        WAIT_FOR_DELIVERY();

        // Select AP
        apic_write_register(LAPIC_REG_INTERRUPT_COMMAND1, v1);

        // DeAssert
        uint32_t v3 = (apic_read_register(LAPIC_REG_INTERRUPT_COMMAND0) & 0xfff00000) | 0x8500;
        apic_write_register(LAPIC_REG_INTERRUPT_COMMAND0, v3);

        WAIT_FOR_DELIVERY();
        sleep(10);

        // Send STARTUP IPI
        for (int j = 0; j < 2; j++)
        {
            apic_write_register(LAPIC_ERR_STATUS_REGISTER, 0);  // Clear APIC errors

            // Select AP
            apic_write_register(LAPIC_REG_INTERRUPT_COMMAND1, v1);

            // Trigger STARTUP IPI
            uint32_t v4 = (apic_read_register(LAPIC_REG_INTERRUPT_COMMAND0) & 0xfff0f800) | 0x608;
            apic_write_register(LAPIC_REG_INTERRUPT_COMMAND0, v4);

            sleep(1);
            WAIT_FOR_DELIVERY();
        }
        
        WAIT_FOR_CORE(i, apicCore);   // Verify the core started running
        
        // Setup core information
        void *apStack = vmm_createIdentityPage(_KernelPML4, VMM_KERNEL_ATTRIBUTES);
        assert(apStack);
        currentContext->lapicID = currentLapicID;
        currentContext->kernelStack = (uint32_t)(uint64_t)apStack + PAGE_SIZE;
        
        apicCore->context = (uint64_t)currentContext;
        apicCore->pml4 = (uint32_t)(uint64_t)_KernelPML4;
        apicCore->stackTop = (uint32_t)(uint64_t)currentContext->kernelStack;
        apicCore->apEntry = (uint64_t)ap_entry;
        apicCore->bspStatus = 1;    // Signal bsp initialization finished
        
        WAIT_FOR_CORE(i, apicCore);   // Verify the core received the status and initialized successfully
    }
    
    LOG("All cores initialized (%u)\n", _MADT.coreCount);
}