#include <io/serial.h>
#include <io/io.h>
#include <logger.h>
#include <assert.h>
#include <panic.h>
#include <gui/screen.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/pic.h>
#include <arch/rsdp.h>
#include <arch/apic/madt.h>
#include <arch/apic/apic.h>
#include <arch/apic/ioapic.h>
#include <arch/apic/timer.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/heap.h>
#include <dev/pit.h>
#include <dev/ps2/kbd.h>
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
    assert(ps2_kbd_init());
    lapic_timer_init();
}

extern int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap && bootInfo->rsdp);
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
    pic_disable();

    // Initialize APIC
    rsdp_init(bootInfo->rsdp);
    madt_init();
    //apic_init();
    //ioapic_init();
    
    // Initialize devices
    dev_init();
    pic_init(IRQ0, IRQ0 + 8, false);
    __STI();
    
    // Initialize filesystem
    ide_init(ATA_DEVICE);   // Initialize disk controller
    ext2_init();            // Initialize root filesystem
    
    // Initialize user-space related 
    syscalls_init();        // Initialize syscalls
    process_init();         // Initialize process management
    scheduler_init();       // Initialize the scheduler

    extern void shell();
    assert(process_create("Shell", shell, PriorityInteractive));
    
    LOG("Kernel initialization finished. Jumping to user space\n\n");
    yield();                // Start executing processes
    
    panic("Unreachable");
}