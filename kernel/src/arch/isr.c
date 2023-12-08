#include <arch/isr.h>
#include <arch/pic.h>
#include <panic.h>
#include <logger.h>

static ISRHandler g_handlers[IDT_ENTRIES];
static bool g_slaveEnabled;

void isr_init()
{
    g_slaveEnabled = false;
    for (uint32_t i = 0; i < IDT_ENTRIES; i++)
        g_handlers[i] = NULL;
}

bool isr_registerHandler(const uint8_t interrupt, const bool autoUnmaskIRQ, ISRHandler handler)
{
    if (g_handlers[interrupt])
        return false;

    // Prepare IRQ interrupt if necessary
    if (interrupt >= IRQ0 && interrupt <= IRQ15)
    {
        uint64_t irqNum = interrupt - IRQ0;
        if (!g_slaveEnabled && irqNum >= 8)
        {
            pic_unmask(IRQ_SLAVE);
            g_slaveEnabled = true;
        }
        
        if (autoUnmaskIRQ)
            pic_unmask(irqNum);
    }

    g_handlers[interrupt] = handler;
    LOG("Registered a handler for interrupt %u\n", interrupt);
    
    return true;
}

extern void isr_interrupt_handler(InterruptStack_t *stack)
{
    uint64_t intNum = stack->interruptNumber;
    if (g_handlers[intNum])
    {
        g_handlers[intNum](stack);
        if (intNum >= IRQ0 && intNum <= IRQ15)
            pic_sendEOI(intNum - IRQ0);
    }
    else
        ipanic(stack, "Unhandled interrupt");
}