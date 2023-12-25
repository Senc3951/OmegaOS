#include <dev/pit.h>
#include <io/io.h>
#include <arch/isr.h>
#include <sys/scheduler.h>
#include <assert.h>
#include <logger.h>

#define TIME 10

volatile size_t g_CountDown;
volatile size_t g_test = TIME;

static void interruptHandler(InterruptStack_t *stack)
{
    if (_CurrentProcess)
    {
        g_test--;
        if (g_test == 0)
        {
            g_test = TIME;
            yield(stack);
        }
    }
    
    g_CountDown--;
}

void pit_init(const uint16_t frequency)
{
    assert(isr_registerHandler(IRQ0, interruptHandler));
    
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
}

void sleep(const size_t milliseconds)
{
    g_CountDown = milliseconds;
    while (g_CountDown > 0)
        __HALT();
}