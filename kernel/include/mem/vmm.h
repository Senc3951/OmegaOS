#pragma once

#include <bootinfo.h>

#define ENTRIES_PER_PAGE_TABLE 512

enum PageAttributes
{
    PA_PRESENT          = 1ULL << 0,
    PA_READ_WRITE       = 1ULL << 1,
    PA_SUPERVISOR       = 1ULL << 2,
    PA_WRITE_THROUGH    = 1ULL << 3,
    PA_CACHE_DISABLED   = 1ULL << 4,
    PA_PAGE_SIZE        = 1ULL << 7,
    PA_GLOBAL           = 1ULL << 8,
    PA_EXECUTE_DISABLED = 1ULL << 63
};

#define VMM_DEFAULT_ATTRIBUTES (PA_PRESENT | PA_READ_WRITE) 

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
        uint64_t pageSize       : 1;
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

/// @brief Load the table into the CR3 register.
/// @param pml4 PML4 table to load.
void vmm_loadTable(PageTable_t *pml4);

/// @brief Get the physical address a virtual one is mapped to.
/// @param virt Virtual address.
/// @return Physical address the virtual one is mapped, NULL, otherwise.
void *virt2phys(void *virt);

/// @brief Map a physical address to a virtual one.
/// @param phys Physical address.
/// @param virt Virtual address.
/// @param attr Attributes to map the address with.
void vmm_mapPage(void *phys, void *virt, const uint64_t attr);

/// @brief Identity map a physical address to a virtual one.
/// @param phys Physical address.
/// @param attr Attributes to map the address with.
void vmm_identityMapPage(void *phys, const uint64_t attr);

/// @brief Remove a page from the page table.
/// @param virt Virtual address to remove from the page table.
/// @return Physical address the page was mapped to, NULL if wasn't mapped.
void *vmm_unmapPage(void *virt);

/// @brief Allocate a virtual page and map it in the page table.
/// @param virt Virtual address to map the page to.
/// @param attr Attributes of the page.
/// @return Virtual address of the page.
void *vmm_getPage(void *virt, const uint64_t attr);

/// @brief Allocate a virtual page and map it in the page table.
/// @param attr Attributes of the page.
/// @return Virtual address of the page.
void *vmm_getIdentityPage(const uint64_t attr);