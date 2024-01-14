#include <mem/vmm.h>
#include <mem/pmm.h>
#include <mem/heap.h>
#include <arch/isr.h>
#include <arch/cpu.h>
#include <sys/syscalls.h>
#include <assert.h>
#include <panic.h>
#include <libc/string.h>
#include <logger.h>

#define FLUSH_TLB(addr) asm volatile("invlpg (%0)" ::"r" (addr) : "memory")

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
    
    virtAddr = RNDWN(virtAddr, PAGE_SIZE);
    PageTable_t *pml4 = _CurrentProcess->pml4;
    if (errCode & PF_PRESENT)
    {
        void *frame = pmm_getFrame();
        assert(frame);
        
        vmm_mapPage(pml4, frame, (void *)virtAddr, VMM_USER_ATTRIBUTES);
        FLUSH_TLB(virtAddr);
        LOG_PROC("Mapped %p to %p\n", virtAddr, frame);
    }
    else
    {
        LOG_PROC("Terminating process because it attempted to access an illegal address %p\n", virtAddr);
        sys_exit(0);
    }
}

void vmm_init(const Framebuffer_t *fb)
{
    assert(isr_registerHandler(PageFault, pageFaultHandler));
    
    // Create the PML4 table.
    assert(_KernelPML4 = (PageTable_t *)pmm_getFrame());
    memset(_KernelPML4, 0, PAGE_SIZE);
    LOG("Kernel PML4 at %p\n", _KernelPML4);
    
    // Identity map entire memory
    uint64_t memEnd = RNDUP(pmm_getMemorySize(), PAGE_SIZE);
    for (uint64_t addr = 0; addr < memEnd; addr += PAGE_SIZE)
        vmm_identityMapPage(_KernelPML4, (void *)addr, VMM_USER_ATTRIBUTES);
    
    // Map the kernel
    uint64_t krnStart = RNDWN(_KernelStart, PAGE_SIZE);
    uint64_t krnEnd = RNDUP(_KernelEnd, PAGE_SIZE);
    uint64_t krnWritableStart = RNDWN(_KernelWritableStart, PAGE_SIZE);
    uint64_t krnWritableEnd = RNDUP(_KernelWritableEnd, PAGE_SIZE);
    for (uint64_t addr = krnStart; addr < krnEnd; addr += PAGE_SIZE)
    {
        if (addr >= krnWritableStart && addr <= krnWritableEnd)
            vmm_identityMapPage(_KernelPML4, (void *)addr, VMM_USER_ATTRIBUTES);
        else
            vmm_identityMapPage(_KernelPML4, (void *)addr, PA_PRESENT | PA_SUPERVISOR);
    }
    
    // Identity map the framebuffer
    uint64_t fbBase = RNDWN((uint64_t)fb->baseAddress, PAGE_SIZE);
    uint64_t fbEnd = RNDUP(fbBase + fb->bufferSize, PAGE_SIZE);
    for (uint64_t addr = fbBase; addr < fbEnd; addr += PAGE_SIZE)
        vmm_identityMapPage(_KernelPML4, (void *)addr, VMM_KERNEL_ATTRIBUTES);
    
    // Load the new table
    vmm_switchTable(_KernelPML4);
}

PageTable_t *vmm_createAddressSpace(PageTable_t *parent)
{
    PageTable_t *pml4 = vmm_createIdentityPage(parent, VMM_USER_ATTRIBUTES);
    assert(pml4);
    memcpy(pml4, parent, PAGE_SIZE);
    
    return pml4;
}

void vmm_destroyAddressSpace(PageTable_t *parent, PageTable_t *pml4)
{
    uint16_t i, j, k, m;
    PageTableEntry_t *entry;
    PageTable_t *pdp, *pd, *pt;
    for (i = 0; i < ENTRIES_PER_PAGE_TABLE; i++)
    {
        if (!pml4->entries[i].present)
            continue;
        
        pdp = (PageTable_t *)(pml4->entries[i].attr.address << 12);
        for (j = 0; j < ENTRIES_PER_PAGE_TABLE; j++)
        {
            if (!pdp->entries[j].present)
                continue;
            
            pd = (PageTable_t *)(pdp->entries[j].attr.address << 12);
            for (k = 0; k < ENTRIES_PER_PAGE_TABLE; k++)
            {
                if (!pd->entries[k].present)
                    continue;
                
                pt = (PageTable_t *)(pd->entries[k].attr.address << 12);
                for (m = 0; m < ENTRIES_PER_PAGE_TABLE; m++)
                {
                    entry = &pt->entries[m];
                    if (!entry->present)
                        continue;
                    
                    void *addr = (void *)(entry->attr.address << 12);
                    pmm_releaseFrame(addr);
                }
            }
        }   
    }
    
    vmm_unmapPage(parent, pml4);
    memset(pml4, 0, PAGE_SIZE);
}

void vmm_switchTable(PageTable_t *pml4)
{
    asm volatile("mov %0, %%cr3" : : "r"(pml4));
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

void *vmm_unmapPages(PageTable_t *pml4, void *virt, const uint64_t pages)
{
    void *p1 = vmm_unmapPage(pml4, virt);
    if (!p1)
        return p1;

    for (uint64_t i = 1; i < pages; i++)
        vmm_unmapPage(pml4, (void *)((uint64_t)virt + i * PAGE_SIZE));
    
    return p1;
}

void *vmm_createPage(PageTable_t *pml4, void *virt, const uint64_t attr)
{
    void *frame = pmm_getFrame();
    if (!frame)
        return NULL;

    vmm_mapPage(pml4, frame, virt, attr);
    FLUSH_TLB(virt);
    
    return frame;
}

void *vmm_createPages(PageTable_t *pml4, void *virt, const uint64_t pages, const uint64_t attr)
{
    void *addr = NULL;
    for (uint64_t i = 0; i < pages; i++)
    {
        if (!(addr = vmm_createPage(pml4, (void *)((uint64_t)virt + i * PAGE_SIZE), attr)))
            return NULL;
    }
    
    return addr;
}

void *vmm_createIdentityPage(PageTable_t *pml4, const uint64_t attr)
{
    void *frame = pmm_getFrame();
    if (!frame)
        return NULL;
    
    vmm_identityMapPage(pml4, frame, attr);
    FLUSH_TLB(frame);
    
    return frame;
}