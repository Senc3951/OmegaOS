#include <assert.h>
#include <arch/apic/apic.h>
#include <io/io.h>
#include <gui/screen.h>

void __NO_RETURN__  __kassertation_failed(const bool early, const char *file, const char *function, const uint64_t line, const char *expr)
{
    __CLI();
    if (_ApicInitialized)
        apic_broadcast_ipi(APIC_DEST_SHORTHAND_ALL_BUT_SELF, IPI_ISR);
        
    
    if (!early)
    {
        screen_clear(Blue);
        kprintf("Assertation failed: %s, function %s, file %s, line %llu\n", expr, function, file, line);
    }
    
    __HCF();
}