#include <dev/pit.h>
#include <arch/isr.h>
#include <sys/scheduler.h>
#include <io/io.h>
#include <assert.h>
#include <logger.h>

#define TIME 10

volatile size_t g_countDown, g_timeTillCS;

static void interruptHandler(InterruptStack_t *stack)
{
    if (currentProcess())
    {
        g_timeTillCS--;
        if (g_timeTillCS == 0)
        {
            g_timeTillCS = TIME;
            
            Process_t *next = dispatch(stack);
            yield(next);
        }
    }
        
    g_countDown--;
}

void pit_init(const uint16_t frequency)
{
    uint16_t freq = 0x1234DE / frequency;
    if ((0x1234DE % frequency) > (frequency / 2))
        freq++;
    
    // Configure the PIT
    outb(PIT_COMMAND_PORT, PIT_CHANNEL0 | PIT_LOHIBYTE | PIT_SQUARE_WAVE_GENERATOR | PIT_BCD);
    __IO_WAIT();
    
    // Configure the frequency
    outb(PIT_CHANNEL_0, freq & 0xFF);               // Low byte
    __IO_WAIT();
    outb(PIT_CHANNEL_0, (freq & 0xFF00) >> 8);      // High byte
    __IO_WAIT();
    
    assert(isr_registerHandler(TIMER_ISR, interruptHandler));
    LOG("PIT timer initialized\n");
}

void pit_sleep_no_int(const size_t ms)
{
    uint64_t total_count = 0x4A9 * ms;
	do
    {
        uint16_t count = MIN(total_count, 0xFFFFU);
        outb(0x43, 0x30);
        outb(0x40, count & 0xFF);
        outb(0x40, count >> 8);
        do
        {
            __PAUSE();
            outb(0x43, 0xE2);
        } while ((inb(0x40) & (1 << 7)) == 0);
        total_count -= count;
	} while ((total_count & ~0xFFFF) != 0);
}

void pit_sleep(const size_t ms)
{
    g_countDown = ms;
    while (g_countDown > 0)
        __PAUSE();
}