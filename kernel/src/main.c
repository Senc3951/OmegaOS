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
    
    kprintf("\ncreating test_file.txt\n");
    int res = vfs_create("test_file.txt", 0);
    if (res != ENOER)
    {
        if (res == EEXIST)
        {
            kprintf("File already exists. Reading file\n");
            VfsNode_t *node = vfs_openFile("test_file.txt", 0);
            if (!node)
                kprintf("Failed opening vfsnode of file\n");
            else
            {
                char buffer[100];
                ssize_t rb = vfs_read(node, 0, 38, buffer);
                if (rb < 0)
                    kprintf("Failed reading from file: %ld\n", rb);
                else
                {
                    buffer[rb] = '\0';
                    kprintf("File content: {%s}\n", buffer);
                }

                kfree(node);
            }
        }
        else
            kprintf("failed creating file\n");
    }
    else
    {
        kprintf("File doesn't exist. created successfully\n");
        
        char buffer[] = "hello world\nthis is a testing buffer!";
        VfsNode_t *node = vfs_openFile("test_file.txt", 0);
        if (!node)
            kprintf("Failed opening vfsnode of file\n");
        else
        {
            kprintf("Writing in bad offset, expected to fail\n");
            ssize_t wb = vfs_write(node, 1, sizeof(buffer), buffer);
            kprintf("Received: %ld\n", wb);
            kprintf("Writing normal\n");
            wb = vfs_write(node, 0, sizeof(buffer), buffer);
            kprintf("Received: %ld\n", wb);

            if (wb < 0)
                kprintf("Writing failed!\n");
        }
        
        kfree(node);
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