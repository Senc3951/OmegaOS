#include <arch/rsdp.h>
#include <mem/vmm.h>
#include <assert.h>
#include <logger.h>
#include <libc/string.h>

static void *g_rsdt;
static uint16_t g_version;
static size_t g_tables;

#define RSDT_TABLE_SIZE 4

static bool validateTable(void *ptr, size_t count)
{
    uint8_t sum = 0;
    char *p = (char *)ptr;
    for (size_t i = 0; i < count; i++)
        sum += p[i];
       
    return sum == 0;
}

static void rsdt_init(void *ptr, uint16_t version)
{
    vmm_identityMapPage(_KernelPML4, (void *)RNDWN((uint64_t)ptr, PAGE_SIZE), VMM_KERNEL_ATTRIBUTES);
    
    g_version = version;
    if (version == 1)
        g_rsdt = (RSDT_t *)ptr;
    else
        g_rsdt = (XSDT_t *)ptr;
    
    assert(validateTable(g_rsdt, ((RSDT_t *)ptr)->header.length));
    LOG("RSDT: %p\n", ptr);
    
    RSDTHeader_t *header = &((RSDT_t *)ptr)->header;
    g_tables = (header->length - sizeof(header)) / 4;
    if (version == 2)
        g_tables /= 2;
    
    for (size_t i = 0; i < g_tables; i++)
    {
        RSDTHeader_t *header;
        if (version == 1)
            header = (RSDTHeader_t *)((RSDT_t *)ptr)->tables[i];
        else
            header = (RSDTHeader_t *)((XSDT_t *)ptr)->tables[i];
                
        assert(validateTable(header, header->length));
    }
}

void rsdp_init(void *ptr)
{
    vmm_identityMapPage(_KernelPML4, (void *)RNDWN((uint64_t)ptr, PAGE_SIZE), VMM_KERNEL_ATTRIBUTES);

    RSDP_t *rsdp = (RSDP_t *)ptr;
    uint16_t version = 2;
    if (rsdp->revision == 0)
        version = 1;
    
    LOG("RSDP: %p. Version: %d\n", ptr, version);
    assert(validateTable(ptr, sizeof(RSDP_t)));
    if (version == 2)
        assert(validateTable(ptr + sizeof(RSDP_t), 14));
    
    if (version == 1)
        rsdt_init((void *)rsdp->rsdtAddress, version);
    else
    {
        XSDP_t *xsdt = (XSDP_t *)ptr;
        rsdt_init((void *)xsdt->xsdtAddress, version);
    }
}

void *rsdt_findTable(const char *name)
{
    for (size_t i = 0; i < g_tables; i++)
    {
        RSDTHeader_t *header;
        if (g_version == 1)
            header = (RSDTHeader_t *)((RSDT_t *)g_rsdt)->tables[i];
        else
            header = (RSDTHeader_t *)((XSDT_t *)g_rsdt)->tables[i];
        
        if (!strncmp(header->signature, name, RSDT_TABLE_SIZE))
            return (void *)header;
    }
    
    return NULL;
}