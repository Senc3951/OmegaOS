#pragma once

#include <bootinfo.h>

enum PageAttributes
{
    PA_PRESENT          = 1ULL << 0,
    PA_READ_WRITE       = 1ULL << 1,
    PA_SUPERVISOR       = 1ULL << 2,
    PA_PWT              = 1ULL << 3,
    PA_PCD              = 1ULL << 4,
    PA_PAT              = 1ULL << 7,
    PA_GLOBAL           = 1ULL << 8,
    PA_EXECUTE_DISABLED = 1ULL << 63
};

#define PA_UNCACHEABLE      (0) /* All accesses are uncacheable. WC is not allowed. */
#define PA_WRITE_COMBINING  (PA_PWT)    /* All access are uncacheable. WC is allowed. */
#define PA_WRITE_THROUGH    (PA_PAT)
#define PA_WRITE_PROTECT    (PA_PAT | PA_PWT)
#define PA_WRITE_BACK       (PA_PAT | PA_PCD)
#define PA_UNCACHED         (PA_PAT | PA_PCD | PA_PWT)

#define ENTRIES_PER_PAGE_TABLE  512
#define VMM_KERNEL_ATTRIBUTES   (PA_PRESENT | PA_READ_WRITE)
#define VMM_USER_ATTRIBUTES     (PA_PRESENT | PA_READ_WRITE | PA_SUPERVISOR) 

/// @brief Entry in the page table.
typedef union PAGE_TABLE_ENTRY
{
    uint64_t raw;
    struct
    {
        uint64_t present        : 1;
        uint64_t readWrite      : 1;
        uint64_t supervisor     : 1;
        uint64_t writeThrough   : 1;
        uint64_t cacheDisabled  : 1;
        uint64_t accessed       : 1;
        uint64_t dirty          : 1;
        uint64_t pat            : 1;
        uint64_t global         : 1;
        uint64_t avl1           : 3;
        uint64_t address        : 40;
        uint64_t avl2           : 7;
        uint64_t pk             : 4;
        uint64_t xd             : 1;
    } __PACKED__ attr;
    #define present attr.present
} __PACKED__ PageTableEntry_t;

typedef struct PAGE_TABLE
{
    PageTableEntry_t entries[ENTRIES_PER_PAGE_TABLE];
} __PACKED__ __ALIGNED__(PAGE_SIZE) PageTable_t;

/// @brief Initialize the virtual memory.
/// @param fb Framebuffer.
void vmm_init(const Framebuffer_t *fb);

/// @brief Destroy an address space.
/// @param parent Parent page table.
/// @param pml4 Page table to destroy.
void vmm_destroyAddressSpace(PageTable_t *parent, PageTable_t *pml4);

/// @brief Create a new address space.
/// @param parent Parent page table.
/// @return New address space.
PageTable_t *vmm_createAddressSpace(PageTable_t *parent);

/// @brief Destroy an address space.
/// @param parent Parent page table.
/// @param pml4 Page table to destroy.
void vmm_destroyAddressSpace(PageTable_t *parent, PageTable_t *pml4);

/// @brief Switch the pml4 table.
/// @param pml4 Table to switch.
void vmm_switchTable(PageTable_t *pml4);

/// @brief Get the physical address a virtual one is mapped to.
/// @param pml4 Table to perform the operation on.
/// @param virt Virtual address.
/// @return Physical address the virtual one is mapped, NULL, otherwise.
void *virt2phys(PageTable_t *pml4, void *virt);

/// @brief Map a physical address to a virtual one.
/// @param pml4 Table to perform the operation on.
/// @param phys Physical address.
/// @param virt Virtual address.
/// @param attr Attributes to map the address with.
void vmm_mapPage(PageTable_t *pml4, void *phys, void *virt, const uint64_t attr);

/// @brief Identity map a physical address to a virtual one.
/// @param pml4 Table to perform the operation on.
/// @param phys Physical address.
/// @param attr Attributes to map the address with.
void vmm_identityMapPage(PageTable_t *pml4, void *phys, const uint64_t attr);

/// @brief Remove a page from the page table.
/// @param pml4 Table to perform the operation on.
/// @param virt Virtual address to remove from the page table.
/// @return Physical address the page was mapped to, NULL if wasn't mapped.
void *vmm_unmapPage(PageTable_t *pml4, void *virt);

/// @brief Remove a page from the page table.
/// @param pml4 Table to perform the operation on.
/// @param virt Virtual address to remove from the page table.
/// @param pages Pages to unmap.
/// @return Physical address the page was mapped to, NULL if wasn't mapped.
void *vmm_unmapPages(PageTable_t *pml4, void *virt, const uint64_t pages);

/// @brief Allocate a virtual page and map it in the page table.
/// @param pml4 Table to perform the operation on.
/// @param virt Virtual address to map the page to.
/// @param attr Attributes of the page.
/// @return Virtual address of the page.
void *vmm_createPage(PageTable_t *pml4, void *virt, const uint64_t attr);

/// @brief Allocate a virtual page and map it in the page table.
/// @param pml4 Table to perform the operation on.
/// @param virt Virtual address to map the page to.
/// @param pages Pages to map.
/// @param attr Attributes of the page.
/// @return Last virtual address of the page.
void *vmm_createPages(PageTable_t *pml4, void *virt, const uint64_t pages, const uint64_t attr);

/// @brief Allocate a virtual page and map it in the page table.
/// @param pml4 Table to perform the operation on.
/// @param attr Attributes of the page.
/// @return Virtual address of the page.
void *vmm_createIdentityPage(PageTable_t *pml4, const uint64_t attr);

/// @brief Create identity mapped pages.
/// @param pml4 Table to perform the operation on.
/// @param pages Pages to map.
/// @param attr Attributes of the page.
/// @return Physical address of the first page, NULL, if failed.
void *vmm_createIdentityPages(PageTable_t *pml4, const uint64_t pages, const uint64_t attr);

extern PageTable_t *_KernelPML4;