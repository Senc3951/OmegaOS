#include <arch/apic/madt.h>
#include <arch/rsdp.h>
#include <mem/vmm.h>
#include <assert.h>
#include <libc/string.h>

typedef struct MADT
{
    RSDTHeader_t header;
    uint32_t lapic;
    uint32_t flags;
} __PACKED__ MADT_t;

MADTInfo_t _MADT;

#define PROCESSOR_LAPIC    0
#define IO_APIC            1
#define IO_APIC_OVERRIDE   2
#define LAPIC_OVERRIDE     5

void madt_init()
{
    MADT_t *madt = (MADT_t *)rsdt_findTable(RSDT_NAME_APIC);
    assert(madt);

    memset(&_MADT, 0, sizeof(_MADT));
    _MADT.lapic = (uint64_t)madt->lapic;
    uint8_t *end = (uint8_t *)madt + madt->header.length;
    uint8_t *ptr = (uint8_t *)madt;

    for (ptr += 44; ptr < end; ptr += ptr[1])
    {
        switch (ptr[0])
        {
            case PROCESSOR_LAPIC:
                if (ptr[4] & 1)
                    _MADT.coreIDs[_MADT.coreCount++] = ptr[3];
                
                break;
            case IO_APIC:
                IOAPIC_t *ioapic = (IOAPIC_t *)(ptr + 2);
                _MADT.ioapics[_MADT.ioapicCount++] = ioapic;
                
                vmm_identityMapPage(_KernelPML4, (void *)RNDWN(ioapic->address, PAGE_SIZE), VMM_KERNEL_ATTRIBUTES);
                break;
            case IO_APIC_OVERRIDE:
                IoApicInterruptOverride_t *override = (IoApicInterruptOverride_t *)(ptr + 2);
                _MADT.ioapicInterruptOverride[_MADT.ioapicInterruptOverrideCount++] = override;
                
                break;
            case LAPIC_OVERRIDE:
                _MADT.lapic = (uint64_t)(ptr + 4);
                break;
        }
    }
    
    vmm_identityMapPage(_KernelPML4, (void *)(RNDWN(_MADT.lapic, PAGE_SIZE)), VMM_KERNEL_ATTRIBUTES);
}