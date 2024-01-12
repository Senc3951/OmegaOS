#include <io/serial.h>
#include <io/io.h>
#include <logger.h>
#include <assert.h>
#include <panic.h>
#include <gui/screen.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/pic.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/heap.h>
#include <dev/pit.h>
#include <dev/storage/ide.h>
#include <fs/ext2.h>
#include <sys/syscalls.h>
#include <sys/process.h>
#include <sys/scheduler.h>

extern uint64_t _kernel_start, _kernel_end, _kernel_writable_start, _kernel_writable_end;
uint64_t _KernelStart, _KernelEnd, _KernelWritableStart, _KernelWritableEnd;

extern void t1();
extern void t2();

void dev_init()
{
    pit_init(PIT_DEFAULT_FREQUENCY);
}

extern int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap);
    eassert(serial_init());
    
    _KernelStart = (uint64_t)&_kernel_start;
    _KernelEnd = (uint64_t)&_kernel_end;
    _KernelWritableStart = (uint64_t)&_kernel_writable_start;
    _KernelWritableEnd = (uint64_t)&_kernel_writable_end;
        
    // Initialize screen
    screen_init(bootInfo->fb, bootInfo->font);
    
    // Initialize memory management
    pmm_init(bootInfo->mmap, bootInfo->mmapSize, bootInfo->mmapDescriptorSize, bootInfo->fb);
    vmm_init(bootInfo->fb);
    heap_init();
    
    // Initialize interrupts
    gdt_load();
    idt_load();
    isr_init();
    pic_init(IRQ0, IRQ0 + 8, false);
    dev_init();
    __STI();
    
    // Initialize filesystem
    ide_init(ATA_DEVICE);   // Initialize disk controller
    ext2_init();            // Initialize root filesystem
    
    // Initialize user-space related 
    syscalls_init();                    // Initialize syscalls
    Process_t *p = process_init();
    scheduler_init(p);   // Initialize required scheduling structs
        
    process_create("p1", t1);
    process_create("p2", t2);
    yield();                // Start executing processes
    
    panic("Unreachable");
}