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

#include <libc/string.h>

extern uint64_t _krnStart, _krnEnd;
uint64_t _KernelStart, _KernelEnd;

static void stage1(BootInfo_t *bootInfo)
{
    _KernelStart = (uint64_t)&_krnStart;
    _KernelEnd = (uint64_t)&_krnEnd;
    LOG("Kernel resides at: 0x%x - 0x%x\n", _KernelStart, _KernelEnd);
    
    // Initialize screen
    screen_init(bootInfo->fb, bootInfo->font);
    LOG("Framebuffer: %p (%ux%ux%u)\n", bootInfo->fb->baseAddress, bootInfo->fb->height, bootInfo->fb->width, bootInfo->fb->bytesPerPixel * 8);
    
    // Enable interrupts
    gdt_load();
    idt_load();
    isr_init();
    pic_init(IRQ0, IRQ0 + 8, false);
    __STI();
    
    // Initialize memory management
    pmm_init(bootInfo->mmap, bootInfo->mmapSize, bootInfo->mmapDescriptorSize, bootInfo->fb);
    vmm_init(bootInfo->fb);
    heap_init(KRN_HEAP_START, KRN_HEAP_SIZE);
    LOG("Kernel heap: %p - %p\n", KRN_HEAP_START, KRN_HEAP_START + KRN_HEAP_SIZE);
}

static void stage2()
{
    // Initialize disk controller
    ide_init(ATA_DEVICE);
    
    // Initialize root filesystem
    ext2_init();

    int i = 0;
    struct dirent *dir;
    kprintf("Iterating /\n");
    while ((dir = vfs_readdir(_RootFS, i++)))
    {
        kprintf("%s %u\n", dir->name, dir->ino);
        kfree(dir);
    }
        
    int res = vfs_create("new.txt", 0);
    int res2 = vfs_mkdir("testdir", 0);
    if (res == ENOER || res2 == ENOER)
    {
        i = 0;
        kprintf("\nReceived: %d %d\n\n", res, res2);   
        while ((dir = vfs_readdir(_RootFS, i++)))
        {
            kprintf("%s %u\n", dir->name, dir->ino);
            kfree(dir);
        }
    }
    else
        kprintf("\nBoth new.txt and testdir already exist\n");
    
    VfsNode_t *newNode = vfs_openFile("notempty", 0);
    if (!newNode)
        kprintf("\nFailed opening directory notempty\n");
    else
    {
        i = 0;
        kprintf("\nIterating notempty\n");
        while ((dir = vfs_readdir(newNode, i++)))
        {
            kprintf("%s %u\n", dir->name, dir->ino);
            kfree(dir);
        }

        kfree(newNode);
    }
    
    kprintf("\nReading from .gitignore\n");
    VfsNode_t *fnode = vfs_openFile(".gitignore", 0);
    if (!fnode)
        kprintf("Failed opening .gitignore\n");
    else
    {
        char buffer[101];
        ssize_t rb = vfs_read(fnode, 2, 100, buffer);
        if (rb < 0)
            kprintf("Received error code %u\n", -rb);
        else
        {
            buffer[rb] = '\0';
            kprintf("Content: {%s}\n", buffer);
        }
        
        kfree(fnode);
    }
}

extern int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap);
    eassert(serial_init());
    
    // Enable core components
    stage1(bootInfo);
    
    // Enable remaining modules
    stage2();
    
    kprintf("Finished initialization\n");
    
    __HCF();
    panic("Unreachable");
}