#include <dev/timer.h>
#include <dev/pit.h>
#include <arch/apic/apic.h>
#include <arch/isr.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

static uint64_t g_ticksPerMS;

#define CALIBRATION_MS  10

static void interruptHandler(InterruptStack_t *stack)
{
    if (currentProcess())
    {
        // Get the next process
        Process_t *next = dispatch(stack);

        // Reschedule the interrupt according to the process time
        lapic_timer_oneshot(next->time);
        
        // Jump to the process
        yield(next);
    }
}

void lapic_timer_init()
{
    if (!_ApicInitialized)
        return;
    
    assert(isr_registerHandler(TIMER_ISR, interruptHandler));

    // Configure the timer
    apic_write_register(LAPIC_TDCR, LAPIC_TIMER_DIVIDER);
    apic_write_register(LAPIC_TICR, UINT32_MAX);
    
    // Sleep & Calculate ticks
    pit_sleep_no_int(CALIBRATION_MS);
    apic_write_register(LAPIC_TIMER, LAPIC_TIMER_MASKED);
    
    // Calculate how many ticks are in 1 ms
    uint32_t apicTicks = apic_read_register(LAPIC_TCCR);
    g_ticksPerMS = (UINT32_MAX - apicTicks) / CALIBRATION_MS;
    LOG("[APIC Timer] Calculated %llu ticks per ms\n", g_ticksPerMS);
}

void lapic_timer_oneshot(const uint64_t ms)
{
    apic_write_register(LAPIC_TDCR, LAPIC_TIMER_DIVIDER);
    apic_write_register(LAPIC_TIMER, LAPIC_TIMER_ONE_SHOT | TIMER_ISR);
    apic_write_register(LAPIC_TICR, g_ticksPerMS * ms);
}

void lapic_timer_periodic(const uint64_t ms)
{    
    apic_write_register(LAPIC_TDCR, LAPIC_TIMER_DIVIDER);
    apic_write_register(LAPIC_TIMER, LAPIC_TIMER_PERIODIC | TIMER_ISR);
    apic_write_register(LAPIC_TICR, g_ticksPerMS * ms);
}