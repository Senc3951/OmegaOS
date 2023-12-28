#include <mem/vmm.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <arch/isr.h>
#include <arch/cpu.h>
#include <sys/scheduler.h>
#include <sys/syscalls.h>
#include <assert.h>
#include <panic.h>
#include <libc/string.h>
#include <logger.h>

#define FLUSH_TLB(addr)     asm volatile("invlpg (%0)" ::"r" (addr) : "memory")
#define FLUSH_TABLE(table)  asm volatile("mov %0, %%cr3" : : "r"(table))

#define ADDRESS_MASK    0x000FFFFFFFFFF000
#define ATTRIBUTES_MASK 0xFFF0000000000FFF

enum PageFaultReason
{
    PF_PRESENT              = 1ULL << 0,
    PF_WRITABLE             = 1ULL << 1,
    PF_USER                 = 1ULL << 2,
    PF_RESERVED             = 1ULL << 3,
    PF_INSTRUCTION_FETCH    = 1ULL << 4,
    PF_PROTECTION_KEY       = 1ULL << 5,
    PF_SHADOW_STACK         = 1ULL << 6,
    PF_SOFTWARE_GUARD       = 1ULL << 15
};

PageTable_t *_KernelPML4 = NULL;

static void setEntry(PageTableEntry_t *entry, uint64_t addr, uint64_t attr)
{
    // Set attributes
    entry->raw &= ADDRESS_MASK;
    entry->raw |= (attr & ATTRIBUTES_MASK);
    
    // Set address
    entry->attr.address = (addr & 0x000000FFFFFFFFFF);
}

static PageTable_t *createEntry(PageTable_t *pt, uint64_t index, const uint64_t attr)
{
    PageTable_t *newPT = (PageTable_t *)pmm_getFrame();
    assert(newPT);
    memset(newPT, 0, PAGE_SIZE);

    PageTableEntry_t *entry = &pt->entries[index];
    setEntry(entry, (uint64_t)newPT >> 12, attr);
    
    return newPT;
}

static void pageFaultHandler(InterruptStack_t *stack)
{
    uint64_t virtAddr = READ_CR2();
    uint64_t errCode = stack->errorCode;
    if (!(errCode & PF_USER))
       ipanic(stack, "Page fault in kernel");
    
    PageTable_t *pml4 = _CurrentProcess->pml4;
    if (errCode & PF_PRESENT)
    {
        void *frame = pmm_getFrame();
        assert(frame);
        
        vmm_mapPage(pml4, frame, (void *)virtAddr, VMM_USER_ATTRIBUTES);
    }
    else
        sys_exit(0);
}

void vmm_init(const Framebuffer_t *fb)
{
    assert(isr_registerHandler(PageFault, pageFaultHandler));
    
    // Create the PML4 table.
    assert(_KernelPML4 = (PageTable_t *)pmm_getFrame());
    memset(_KernelPML4, 0, PAGE_SIZE);
    LOG("Kernel PML4: %p\n", _KernelPML4);

    // Identity map entire memory
    uint64_t memEnd = RNDUP(pmm_getMemorySize(), PAGE_SIZE);
    for (uint64_t addr = 0; addr < memEnd; addr += PAGE_SIZE)
        vmm_identityMapPage(_KernelPML4, (void *)addr, VMM_USER_ATTRIBUTES);
    
    // Identity map the framebuffer
    uint64_t fbBase = (uint64_t)fb->baseAddress;
    uint64_t fbEnd = RNDUP(fbBase + fb->bufferSize, PAGE_SIZE);
    for (uint64_t addr = fbBase; addr < fbEnd; addr += PAGE_SIZE)
        vmm_identityMapPage(_KernelPML4, (void *)addr, VMM_KERNEL_ATTRIBUTES);
    
    FLUSH_TABLE(_KernelPML4);
}

PageTable_t *vmm_createAddressSpace()
{
    PageTable_t *pml4 = (PageTable_t *)pmm_getFrame();
    assert(pml4);
    memcpy(pml4, _KernelPML4, PAGE_SIZE);
    
    return pml4;
}

void *virt2phys(PageTable_t *pml4, void *virt)
{
    uint64_t uvirt = (uint64_t)virt;
    uint64_t pml4Index = (uvirt >> 39) & 0x1FF;
    if (!pml4->entries[pml4Index].present)
        return NULL;

    PageTable_t *pdp = (PageTable_t *)(pml4->entries[pml4Index].attr.address << 12);
    uint64_t pdpIndex = (uvirt >> 30) & 0x1FF;
    if (!pdp->entries[pdpIndex].present)
        return NULL;

    PageTable_t *pd = (PageTable_t *)(pdp->entries[pdpIndex].attr.address << 12);
    uint64_t pdIndex = (uvirt >> 21) & 0x1FF;
    if (!pd->entries[pdIndex].present)
        return NULL;

    PageTable_t *pt = (PageTable_t *)(pd->entries[pdIndex].attr.address << 12);
    uint64_t ptIndex = (uvirt >> 12) & 0x1FF;
    if (!pt->entries[ptIndex].present)
        return NULL;
    
    return (void *)(pt->entries[ptIndex].attr.address << 12);
}

void vmm_mapPage(PageTable_t *pml4, void *phys, void *virt, const uint64_t attr)
{
    uint64_t uphys = (uint64_t)phys;
    uint64_t uvirt = RNDWN((uint64_t)virt, PAGE_SIZE);
    
    uint64_t pml4Index = (uvirt >> 39) & 0x1FF;
    PageTable_t *pdp;
    if (!pml4->entries[pml4Index].present)
        pdp = createEntry(pml4, pml4Index, attr);
    else
        pdp = (PageTable_t *)(pml4->entries[pml4Index].attr.address << 12);

    uint64_t pdpIndex = (uvirt >> 30) & 0x1FF;
    PageTable_t *pd;
    if (!pdp->entries[pdpIndex].present)
        pd = createEntry(pdp, pdpIndex, attr);
    else
        pd = (PageTable_t *)(pdp->entries[pdpIndex].attr.address << 12);

    uint64_t pdIndex = (uvirt >> 21) & 0x1FF;
    PageTable_t *pt;
    if (!pd->entries[pdIndex].present)
        pt = createEntry(pd, pdIndex, attr);
    else
        pt = (PageTable_t *)(pd->entries[pdIndex].attr.address << 12);
    
    uint64_t ptIndex = (uvirt >> 12) & 0x1FF;
    setEntry(&pt->entries[ptIndex], uphys >> 12, attr);
    
    FLUSH_TLB(virt);
}

void vmm_identityMapPage(PageTable_t *pml4, void *phys, const uint64_t attr)
{
    vmm_mapPage(pml4, phys, phys, attr);
}

void *vmm_unmapPage(PageTable_t *pml4, void *virt)
{
    uint64_t uvirt = (uint64_t)virt;
    uint64_t pml4Index = (uvirt >> 39) & 0x1FF;
    if (!pml4->entries[pml4Index].present)
        return NULL;

    PageTable_t *pdp = (PageTable_t *)(pml4->entries[pml4Index].attr.address << 12);
    uint64_t pdpIndex = (uvirt >> 30) & 0x1FF;
    if (!pdp->entries[pdpIndex].present)
        return NULL;

    PageTable_t *pd = (PageTable_t *)(pdp->entries[pdpIndex].attr.address << 12);
    uint64_t pdIndex = (uvirt >> 21) & 0x1FF;
    if (!pd->entries[pdIndex].present)
        return NULL;

    PageTable_t *pt = (PageTable_t *)(pd->entries[pdIndex].attr.address << 12);
    uint64_t ptIndex = (uvirt >> 12) & 0x1FF;
    if (!pt->entries[ptIndex].present)
        return NULL;
    
    pt->entries[ptIndex].present = 0;
    void *phys = (void *)(pt->entries[ptIndex].attr.address << 12);
    pmm_releaseFrame(phys);
    
    return phys;
}

void *vmm_getPage(PageTable_t *pml4, void *virt, const uint64_t attr)
{
    void *frame = pmm_getFrame();
    if (!frame)
        return NULL;

    vmm_mapPage(pml4, frame, virt, attr);
    return frame;
}

void *vmm_getIdentityPage(PageTable_t *pml4, const uint64_t attr)
{
    void *frame = pmm_getFrame();
    if (!frame)
        return NULL;
    
    vmm_identityMapPage(pml4, frame, attr);
    return frame;
}