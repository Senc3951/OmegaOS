#include <io/serial.h>
#include <io/io.h>
#include <logger.h>
#include <assert.h>
#include <panic.h>
#include <gui/screen.h>
#include <arch/rsdp.h>
#include <arch/apic/madt.h>
#include <arch/apic/apic.h>
#include <arch/apic/ioapic.h>
#include <arch/smp.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/pic.h>
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

extern void x64_enable_cpu_features();

static void dev_init()
{
    pit_init(PIT_DEFAULT_FREQUENCY);
    ps2_kbd_init();
}

int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap && bootInfo->rsdp);
    eassert(serial_init());
    
    // Enable basic cpu features such as SSE
    x64_enable_cpu_features();

    _KernelStart = (uint64_t)&_kernel_start;
    _KernelEnd = (uint64_t)&_kernel_end;
    _KernelWritableStart = (uint64_t)&_kernel_writable_start;
    _KernelWritableEnd = (uint64_t)&_kernel_writable_end;
    LOG("Kernel resides at: %p - %p. Writable: %p - %p\n", _KernelStart, _KernelEnd, _KernelWritableStart, _KernelWritableEnd);
    
    // Initialize screen
    screen_init(bootInfo->fb, bootInfo->font);
    
    // Initialize memory management
    pmm_init(bootInfo->mmap, bootInfo->mmapSize, bootInfo->mmapDescriptorSize, bootInfo->fb);
    vmm_init(bootInfo->fb);
    heap_init();
    
    // Initialize system related & interrupts
    rsdp_init(bootInfo->rsdp);
    gdt_load();
    idt_load();
    isr_init();
    pic_init(IRQ0, IRQ0 + 8, false);
    dev_init();
    __STI();
    
    // Initialize filesystem
    ide_init(ATA_DEVICE);   // Initialize disk controller
    ext2_init();            // Initialize root filesystem
    
    // Initialize APIC & IO APIC
    madt_init();
    apic_init();
    ioapic_init();

    // Initialize other cores    
    smp_init();
    
    // Initialize user-space related 
    tss_late_set();     // Add stacks to the TSS
    syscalls_init();    // Initialize syscalls
    process_init();     // Initialize process related structs and init process
    scheduler_init();   // Initialize required scheduling structs
    
    yield();            // Start executing processes
    
    panic("Unreachable");
}

int ap_entry(CoreContext_t *context)
{
    LOG("[AP] Core %u initialized. Stack: %p\n", context->lapicID, context->kernelStack);
    
    x64_enable_cpu_features();
    // initialize apic, io apic, tss
    // start scheduling
    
    while (1) __HALT();
}