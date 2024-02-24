#include <arch/apic/ipi.h>
#include <arch/apic/apic.h>
#include <dev/pit.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

static bool _IpiInitialized = false;

typedef union APIC_ICR
{
    struct
    {
        uint32_t vector         : 8;
        uint32_t delvMode       : 3;
        uint32_t destMode       : 1;
        uint32_t delvStatus     : 1;
        uint32_t reserved0      : 1;
        uint32_t level          : 1;
        uint32_t trigger        : 1;
        uint32_t reserved1      : 2;
        uint32_t destShorthand  : 2;
        uint32_t reserved2      : 12;
        
        uint32_t reserved3      : 24;
        uint32_t dest           : 8;
	} __PACKED__;
    struct
    {
        uint32_t lo;
        uint32_t hi;
    } __PACKED__;
    uint64_t raw;
} ApicICR_t;

static void interruptHandler(InterruptStack_t *stack)
{
    UNUSED(stack);
    
    __CLI();
    LOG("[Core %u] Received abort ipi\n", currentCPU()->id);
    __HCF();
}

static void __apic_ipi(const int destID, const uint8_t mode, const uint8_t vector, const uint8_t dest, const uint8_t level, const uint8_t trigger)
{
    if (!_ApicInitialized || _CoreCount == 1 || !_IpiInitialized)
        return;
    
    ApicICR_t icr = { 0 };
    icr.vector = vector;
    icr.delvMode = mode;
    
    switch (dest)
    {
        case IRQ_BROADCAST:
            icr.destShorthand = APIC_DEST_SHORTHAND_ALL_BUT_SELF;
            break;
        case IRQ_SINGLE:
            icr.destMode = APIC_DESTMODE_PHYSICAL;
            icr.dest = destID;
            break;
        default:
            assert(false);
    }
    
    icr.level = level;
    icr.trigger = trigger;
    
    apic_write_register(LAPIC_ICRHI, icr.hi);
    apic_write_register(LAPIC_ICRLO, icr.lo);
}

void ipi_send_core(const uint8_t destID, const uint8_t mode, const uint8_t vector)
{
    __apic_ipi(destID, mode, vector, IRQ_SINGLE, APIC_LEVEL_ASSERT, APIC_TRIGGER_EDGE);
}

void ipi_send_deassert(const uint8_t destID, const uint8_t mode, const uint8_t vector)
{
    __apic_ipi(destID, mode, vector, IRQ_SINGLE, APIC_LEVEL_DEASSERT, APIC_TRIGGER_LEVEL);
}

void ipi_broadcast(const uint8_t mode, const uint8_t vector)
{
    __apic_ipi(0, mode, vector, IRQ_BROADCAST, APIC_LEVEL_ASSERT, APIC_TRIGGER_EDGE);
}

bool ipi_wait_accept()
{
    static const uint8_t RETRIES = 10;;
    
    ApicICR_t icr = { 0 };
    for (uint8_t i = 0; i < RETRIES; i++)
    {
        icr.lo = apic_read_register(LAPIC_ICRLO);
        if (icr.delvStatus == 0)
            return true;

        pit_sleep_no_int(1);
    }

    return false;
}

void ipi_init()
{
    if (!_IpiInitialized)
    {
        assert(isr_registerHandler(IPI_ISR, interruptHandler));
        _IpiInitialized = true;
    }
}