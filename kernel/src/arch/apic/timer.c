#include <arch/apic/timer.h>
#include <arch/apic/apic.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

static uint64_t g_ticksPerMS;
volatile bool g_doneSleeping;

#define CALIBRATION_MS      10

#define CONFIGURE_PIT(ms) ({ \
    uint8_t val = inb(0x61) & ~1;               \
    outb(0x61, val);                            \
    \
    outb(0x43, 0b00001110);                     \
    uint32_t counter = 1193182 / (1000 / ms);   \
    outb(0x42, counter);                        \
    outb(0x42, counter >> 8);                   \
})

#define PIT_SLEEP() ({ \
    uint8_t val = (inb(0x61) | 1) & ~2; \
    outb(0x61, val);                    \
    while ((inb(0x61) & 0x20) == 0)     \
        __PAUSE();                      \
})

#define SET_MASK(mask) ({ \
    if (mask)   \
        apic_write_register(LAPIC_TIMER, apic_read_register(LAPIC_TIMER) | LAPIC_TIMER_MASKED);     \
    else        \
        apic_write_register(LAPIC_TIMER, apic_read_register(LAPIC_TIMER) & ~LAPIC_TIMER_MASKED);    \
})

static void interruptHandler(InterruptStack_t *stack)
{
    g_doneSleeping = true;
    if (currentProcess())
    {
        Process_t *next = dispatch(stack);
        yield(next);
    }
}

void lapic_timer_init()
{
    assert(isr_registerHandler(TIMER_ISR, interruptHandler));

    // Configure the PIC
    CONFIGURE_PIT(CALIBRATION_MS);

    // Configure the timer
    apic_write_register(LAPIC_TDCR, LAPIC_TIMER_DIVIDER);
    apic_write_register(LAPIC_TICR, UINT32_MAX);
    SET_MASK(false);
    
    PIT_SLEEP();
    SET_MASK(true);
    
    // Calculate how many ticks are in 1 ms
    uint32_t apicTicks = apic_read_register(LAPIC_TCCR);
    g_ticksPerMS = (UINT32_MAX - apicTicks) / CALIBRATION_MS;
    LOG("[APIC Timer] Calculated %llu ticks per ms\n", g_ticksPerMS);
    LOG("APIC timer initialized\n");
}

void lapic_timer_start(const uint8_t isr)
{
    if (!_ApicInitialized)
        return;
    
    lapic_timer_periodic(isr, 10000000);
}

void lapic_timer_oneshot(const uint8_t isr, const uint64_t ms)
{
    uint32_t arg = LAPIC_TIMER_ONE_SHOT | isr;

    SET_MASK(true);
    apic_write_register(LAPIC_TDCR, LAPIC_TIMER_DIVIDER);
    apic_write_register(LAPIC_TIMER, ((apic_read_register(LAPIC_TIMER) & ~LAPIC_TIMER_MODE_MASK) & 0xFFFFFF00) | arg);
    apic_write_register(LAPIC_TICR, g_ticksPerMS * ms);
    SET_MASK(false);
}

void lapic_timer_periodic(const uint8_t isr, const uint64_t ms)
{
    uint32_t arg = LAPIC_TIMER_PERIODIC | isr;
    
    SET_MASK(true);
    apic_write_register(LAPIC_TDCR, LAPIC_TIMER_DIVIDER);
    apic_write_register(LAPIC_TIMER, ((apic_read_register(LAPIC_TIMER) & ~LAPIC_TIMER_MODE_MASK) & 0xFFFFFF00) | arg);
    apic_write_register(LAPIC_TICR, ms);
    SET_MASK(false);
}

void lapic_timer_msleep(const uint64_t ms)
{
    lapic_timer_oneshot(TIMER_ISR, ms * 10);
    g_doneSleeping = false;
    
    while (!g_doneSleeping)
        __PAUSE();
    
    // REMOVE:
    lapic_timer_start(TIMER_ISR);
}