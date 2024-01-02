#include <arch/cpu.h>
#include <logger.h>

void cpu_init()
{
    char name[13] = { 0 };
    uint32_t unused;
    __cpuid(0, &unused, (uint32_t *)name, (uint32_t *)&name[8], (uint32_t *)&name[4]);
    LOG("CPU: %s\n", name);
}

void __cpuid(uint32_t code, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    asm volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a" (code));
}

void __rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
void __wrmsr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}