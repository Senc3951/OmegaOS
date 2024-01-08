#include <arch/cpu.h>
#include <arch/apic/apic.h>
#include <arch/idt.h>
#include <io/io.h>
#include <assert.h>

extern int x64_enable_cpu_features();

void __rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
void __wrmsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void core_init(CoreContext_t *context)
{
    eassert(x64_enable_cpu_features());
    
    if (context)
    {
        // init gdt
        idt_load();
        apic_ap_init();
        // init ioapic

        //__STI();
    }
}