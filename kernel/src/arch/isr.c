#include <arch/isr.h>
#include <arch/pic.h>
#include <arch/gdt.h>
#include <arch/apic/apic.h>
#include <arch/apic/ioapic.h>
#include <sys/syscalls.h>
#include <panic.h>
#include <logger.h>

static ISRHandler g_handlers[IDT_ENTRIES] = { 0 };
static bool g_slaveEnabled = false;

bool isr_registerHandler(const uint8_t interrupt, ISRHandler handler)
{
    g_handlers[interrupt] = handler;
    if (interrupt >= IRQ0 && interrupt <= IRQ15)
    {
        uint64_t irqNum = interrupt - IRQ0;
        if (_ApicInitialized)
            ioapic_map_irq(irqNum, interrupt, false);
        else
        {
            if (!g_slaveEnabled && irqNum >= 8)
            {
                pic_unmask(IRQ_SLAVE);
                g_slaveEnabled = true;
            }
            
            pic_unmask(irqNum);
        }
    }
    
    LOG("Registered a handler for interrupt %u\n", interrupt);
    return true;
}

extern void isr_interrupt_handler(InterruptStack_t *stack)
{
    uint64_t intNum = stack->interruptNumber;
    if (g_handlers[intNum])
    {
        if (intNum >= IRQ0 && intNum <= IRQ15)
        {
            if (_ApicInitialized)
                apic_send_eoi();
            else
                pic_sendEOI(intNum - IRQ0);
        }
        
        g_handlers[intNum](stack);
    }
    else if (isUserInterrupt(stack))
    {
        LOG_PROC("Terminating process because interrupt 0x%x occurred (0x%x).\n", intNum, stack->errorCode);
        sys_exit(0);
    }
    else
        ipanic(stack, "Unhandled interrupt");
}

bool isUserInterrupt(InterruptStack_t *stack)
{
    return stack->cs == GDT_USER_CS && stack->ds == GDT_USER_DS;
}