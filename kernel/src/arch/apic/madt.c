#include <arch/apic/madt.h>
#include <arch/rsdp.h>
#include <mem/vmm.h>
#include <assert.h>

typedef struct MADT
{
    RSDTHeader_t header;
    uint32_t lapic;
    uint32_t flags;
} __PACKED__ MADT_t;

MADTInfo_t _MADT;

#define MADT_PROCESSOR_LAPIC    0
#define MADT_IO_APIC            1
#define MADT_LAPIC_OVERRIDE     5

void madt_init()
{
    MADT_t *madt = (MADT_t *)rsdt_findTable(RSDT_NAME_APIC);
    assert(madt);

    _MADT.lapic = (uint64_t)madt->lapic;
    uint8_t *end = (uint8_t *)madt + madt->header.length;
    uint8_t *ptr = (uint8_t *)madt;

    for (ptr += 44; ptr < end; ptr += ptr[1])
    {
        switch (ptr[0])
        {       
            case MADT_PROCESSOR_LAPIC:
                if (ptr[4] & 1 || ptr[4] & 2)
                    _MADT.coreIDs[_MADT.coreCount++] = ptr[3];
                
                break;
            case MADT_IO_APIC:
                IOAPIC_t ioapic = { .id = ptr[2], .address = (uint64_t)(ptr + 4), .gsib = (uint64_t)(ptr + 8) };
                _MADT.ioapics[_MADT.ioApicCount++] = ioapic;
                
                vmm_identityMapPage(_KernelPML4, (void *)RNDWN(ioapic.address, PAGE_SIZE), VMM_KERNEL_ATTRIBUTES);
                break;
            case MADT_LAPIC_OVERRIDE:
                _MADT.lapic = (uint64_t)(ptr + 4);
                break;
        }
    }
    
    vmm_identityMapPage(_KernelPML4, (void *)(RNDWN(_MADT.lapic, PAGE_SIZE)), VMM_KERNEL_ATTRIBUTES);
}