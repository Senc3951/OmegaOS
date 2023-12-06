#include <io/io.h>
#include <assert.h>
#include <gui/screen.h>

extern uint64_t _krnStart, _krnEnd;
uint64_t _KernelStart, _KernelEnd;

static void stage1(BootInfo_t *bootInfo)
{
    _KernelStart = (uint64_t)&_krnStart;
    _KernelEnd = (uint64_t)&_krnEnd;

    // Enable screen
    screen_init(bootInfo->fb, bootInfo->font);
}

extern int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap);
    
    // Enable core components
    stage1(bootInfo);
    kprintf("test: %d\n", 234);
    
    __HCF();
}