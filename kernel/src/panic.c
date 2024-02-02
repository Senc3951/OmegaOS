#include <panic.h>
#include <arch/apic/apic.h>
#include <gui/screen.h>
#include <io/io.h>

void __NO_RETURN__  __kpanic(const char *file, const char *function, const uint64_t line, const char *fmt, ...)
{
    __CLI();
    if (_ApicInitialized)
        apic_broadcast_ipi(APIC_DEST_SHORTHAND_ALL_BUT_SELF, IPI_ISR);
    
    va_list va;
    va_start(va, fmt);
    
    screen_clear(Blue);
    screen_puts("Panic: ");
    kvprintf(va, fmt);
    kprintf(", function %s, file %s, line %llu\n", function, file, line);
    va_end(va);
    
    __HCF();
}

void __NO_RETURN__  __ikpanic(InterruptStack_t *stack, const char *fmt, ...)
{
    __CLI();
    if (_ApicInitialized)
        apic_broadcast_ipi(APIC_DEST_SHORTHAND_ALL_BUT_SELF, IPI_ISR);
    
    va_list va;
    va_start(va, fmt);

    screen_clear(Blue);
    screen_puts("Panic: ");
    kvprintf(va, fmt);
    kprintf(". Interrupt: 0x%04x, Error Code: 0x%016x\n", stack->interruptNumber, stack->errorCode);
    va_end(va);
        
    __HCF();
}