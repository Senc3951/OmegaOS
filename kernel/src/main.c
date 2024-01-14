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
    isr_init();
    pmm_init(bootInfo->mmap, bootInfo->mmapSize, bootInfo->mmapDescriptorSize, bootInfo->fb);
    vmm_init(bootInfo->fb);
    heap_init();
    
    // Initialize interrupts
    gdt_load();
    idt_load();
    pic_init(IRQ0, IRQ0 + 8, false);
    dev_init();
    __STI();
    
    // Initialize filesystem
    ide_init(ATA_DEVICE);   // Initialize disk controller
    ext2_init();            // Initialize root filesystem
    
    // Initialize user-space related 
    syscalls_init();        // Initialize syscalls
    process_init();         // Initialize process management
    scheduler_init();       // Initialize the scheduler
    
    extern void shell();
    process_create("Shell", shell, PriorityInteractive);
    
    LOG("Kernel initialization finished. Jumping to user space\n\n");
    yield();                // Start executing processes
    
    panic("Unreachable");
}