#include <bootinfo.h>

extern uint64_t _krnStart, _krnEnd;
uint64_t _KernelStart, _KernelEnd;

extern int _entry(BootInfo_t *bootInfo)
{
    while (1)
        asm volatile("hlt");
}