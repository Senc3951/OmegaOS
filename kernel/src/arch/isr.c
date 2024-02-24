#include <arch/isr.h>
#include <arch/pic.h>
#include <arch/gdt.h>
#include <arch/apic/apic.h>
#include <arch/apic/ioapic.h>
#include <syscall/syscalls.h>
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

static void sendEOI(const uint8_t isr)
{
    if (isr >= IRQ0 && isr <= IRQ15)
    {
        if (_ApicInitialized)
            apic_eoi();
        else
            pic_eoi(isr - IRQ0);
    }
}

extern void isr_interrupt_handler(InterruptStack_t *stack)
{
    uint64_t intNum = stack->interruptNumber;
    if (g_handlers[intNum])
    {
        if (intNum == TIMER_ISR)
        {
            sendEOI(intNum);
            g_handlers[intNum](stack);
        }
        else
        {
            g_handlers[intNum](stack);
            sendEOI(intNum);
        }        
    }
    else if (isUserInterrupt(stack))
    {
        sys_exit(0);
        LOG_PROC("Terminated process because interrupt 0x%x occurred (0x%x).\n", intNum, stack->errorCode);
    }
    else
        ipanic(stack, "Unhandled interrupt");
}

bool isUserInterrupt(InterruptStack_t *stack)
{
    return stack->cs == GDT_USER_CS && stack->ds == GDT_USER_DS;
}