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
#include <scheduler/syscalls.h>
#include <scheduler/process.h>
#include <scheduler/scheduler.h>

extern uint64_t _krnStart, _krnEnd;
uint64_t _KernelStart, _KernelEnd;

void p1()
{
    static int x = 0;
    while(1) x++;
}

void p2()
{
    static int x = 0;
    while(1) x++;
}

extern int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap);
    eassert(serial_init());
    
    _KernelStart = (uint64_t)&_krnStart;
    _KernelEnd = (uint64_t)&_krnEnd;
    LOG("Kernel resides at: 0x%x - 0x%x\n", _KernelStart, _KernelEnd);
    
    // Initialize screen
    screen_init(bootInfo->fb, bootInfo->font);
    
    // Initialize interrupts
    gdt_load();
    idt_load();
    isr_init();
    pic_init(IRQ0, IRQ0 + 8, false);
    __STI();
    
    // Initialize memory management
    pmm_init(bootInfo->mmap, bootInfo->mmapSize, bootInfo->mmapDescriptorSize, bootInfo->fb);
    vmm_init(bootInfo->fb);
    heap_init(KRN_HEAP_START, KRN_HEAP_SIZE);
    
    // Initialize filesystem
    ide_init(ATA_DEVICE);   // Initialize disk controller
    ext2_init();            // Initialize and mount root filesystem
    
    // Initialize user-space related 
    pit_init(PIT_DEFAULT_FREQUENCY);
    tss_late_set();         // Add stacks to the TSS
    syscalls_init();        // Initialize syscalls
    
    process_init();         // Create the idle process
    void* s1=kmalloc(4096);
    void *s2 = kmalloc(4096);
    process_create("p1", p1, s1,4096);
    process_create("p2",p2,s2,4096);
    scheduler_init();       // Start scheduling processes, should never return
    
    panic("Unreachable");
}