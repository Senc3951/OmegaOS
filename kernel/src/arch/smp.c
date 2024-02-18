#include <arch/smp.h>
#include <arch/apic/madt.h>
#include <arch/apic/apic.h>
#include <arch/apic/timer.h>
#include <mem/vmm.h>
#include <io/io.h>
#include <assert.h>
#include <panic.h>
#include <libc/string.h>
#include <logger.h>

typedef struct CORE_LAUNCH_INFO
{
    uint8_t apStatus;
    uint8_t bspStatus;
    uint32_t pml4;
    uint64_t stackTop;
    uint64_t apEntry;
    uint64_t context;
} __PACKED__ CoreLaunchInfo_t;

uint32_t _CoreCount;
CoreContext_t *_Cores;

#define AP_TIMEOUT_RETRY    10
#define AP_TIMEOUT_MS       5

#define WAIT_FOR_CORE(i, core) ({                               \
    for (int j = 0; j < AP_TIMEOUT_RETRY; j++) {                \
        if (core->apStatus == 1)                                \
            break;                                              \
        \
        __PAUSE();                                              \
        lapic_timer_msleep(AP_TIMEOUT_MS);                      \
    }                                                           \
    \
    if (core->apStatus != 1)                                    \
        panic("Failed initializing core %u", _MADT.coreIDs[i]); \
})

void smp_init()
{
    extern int ap_entry(CoreContext_t *context);

    // Copy the AP trampoline code to a fixed address
    extern void trampoline_code();
    extern void trampoline_data();
    void *trampolineCodePtr = (void *)&trampoline_code;
    void *trampolineDataPtr = (void *)&trampoline_data;
    
    vmm_identityMapPage(_KernelPML4, (void *)AP_TRAMPOLINE, VMM_KERNEL_ATTRIBUTES);
    memcpy((void *)AP_TRAMPOLINE, trampolineCodePtr, PAGE_SIZE);
    CoreLaunchInfo_t *coreInfo = (CoreLaunchInfo_t *)(AP_TRAMPOLINE + ((uint64_t)trampolineDataPtr - (uint64_t)trampolineCodePtr));

    // Initialize cores
    for (uint32_t i = 0; i < _CoreCount; i++)
    {
        uint32_t cid = _MADT.coreIDs[i];
        if (cid == _BspID)
            continue;
        
        CoreContext_t *currentContext = (CoreContext_t *)&_Cores[i];
        coreInfo->apStatus = coreInfo->bspStatus = 0;
        
        // Wakeup the core
        LOG("[BSP] Waking up core %u\n", cid);
        apic_wakeup_core(cid);
        WAIT_FOR_CORE(i, coreInfo);
        LOG("[BSP] Core %u responding\n", cid);
        
        // Initialize core
        currentContext->id = cid;
        currentContext->currentProcess = NULL;
        void *kstack = vmm_createIdentityPages(_KernelPML4, CORE_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
        assert(kstack);
        currentContext->stack = (uint64_t)kstack + CORE_STACK_SIZE;
                
        coreInfo->context = (uint64_t)currentContext;
        coreInfo->pml4 = (uint32_t)(uint64_t)_KernelPML4;
        coreInfo->stackTop = (uint32_t)currentContext->stack;
        coreInfo->apEntry = (uint64_t)ap_entry;
        coreInfo->bspStatus = 1;    // Signal bsp initialization finished
        
        LOG("[BSP] Core %u initialized with stack at %p - %p\n", cid, kstack, (uint64_t)kstack + CORE_STACK_SIZE);    
    }
    
    LOG("[BSP] All cores initialized (%u)\n", _CoreCount);
}