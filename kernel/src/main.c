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
#include <drivers/storage/ide.h>
#include <fs/ext2.h>

extern uint64_t _krnStart, _krnEnd;
uint64_t _KernelStart, _KernelEnd;

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

    // Initialize disk controller
    ide_init(ATA_DEVICE);
    
    // Initialize root filesystem
    ext2_init();
    
    kprintf("Finished initialization\n");
    __HCF();
    panic("Unreachable");
}