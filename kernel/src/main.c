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

    char fn[] = "name_for_test_file_x";
    char dn[] = "name_for_test_dir_x";
    for (int i = 0; i < 10; i++)
    {
        fn[19] = '0' + i;
        dn[18] = '0' + i;
        int fr = vfs_create(fn, 0);
        int dr = vfs_mkdir(dn, 0);
        kprintf("file: %d, dir: %d\n", fr, dr);
    }
    
    VfsNode_t *node = vfs_openFile("name_for_test_file_3", 0);
    char buf[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer tempus euismod dignissim. Phasellus et velit et sem ultricies varius. Aenean justo tellus, iaculis vel nulla in, placerat dictum odio. Nulla quam purus, semper vel enim egestas, imperdiet ornare leo. Donec ac augue id justo ultricies blandit ac a mauris. Aliquam cursus ante elit, eget tristique orci convallis quis. Praesent id justo luctus, varius risus eget, hendrerit nisl. Proin laoreet ligula id bibendum pharetra. Morbi feugiat ex arcu, quis sollicitudin neque dignissim ut. Aenean scelerisque lobortis porttitor. Etiam est dolor, sollicitudin in tempus at, dapibus sit amet ante. Nulla ex lacus, facilisis quis vulputate nec, finibus ut quam. Ut imperdiet elit in bibendum tristique. Integer non dolor nulla."
"Aliquam erat volutpat. Proin sit amet odio vitae urna convallis viverra at ac libero. Etiam dignissim sodales enim non pretium. Vivamus eget efficitur sem. Phasellus id bibendum leo. Vivamus vel aliquet ex, vel molestie ligula. Maecenas scelerisque libero non enim consequat accumsan. Ut commodo ex non nibh auctor, in placerat tortor pretium. Vestibulum tristique sed diam et dignissim. Duis dapibus eu enim in dapibus. Mauris id enim vitae eros consequat congue. In bibendum nibh lacus, in faucibus ante sagittis semper. Nunc eget magna mollis, consectetur lorem a, scelerisque risus. Aenean ac orci in eros ultrices ultricies. Phasellus nec dolor a quam pellentesque bibendum."
"Phasellus et sapien venenatis, elementum est at, aliquam orci. Curabitur sem risus, scelerisque sit amet est vel, luctus blandit neque. Phasellus fringilla vel leo ut maximus. Vivamus at diam aliquam, molestie augue imperdiet, hendrerit dolor. Nam lacinia lobortis urna, non pellentesque mauris lacinia non. Maecenas mattis, elit eu tempor imperdiet, sapien lacus ornare ante, in vulputate metus nibh nec mi. Duis non pulvinar metus. Praesent eu eros et ante pharetra vestibulum vehicula quis tortor. Nulla ac pharetra neque."
"Curabitur dui eros, placerat ut metus ac, ultrices dignissim nisl. Mauris sed consectetur quam. Donec at ipsum in ligula euismod blandit et a tortor. Interdum et malesuada fames ac ante ipsum primis in faucibus. Etiam venenatis turpis et volutpat ultricies. Morbi quis condimentum tellus. Vivamus dignissim orci tincidunt nulla ornare, nec blandit magna mattis. Vestibulum vel massa eu eros gravida fringilla. Vivamus in urna id nisl placerat volutpat at eu massa. Fusce eu justo eu massa molestie pharetra. Phasellus scelerisque, lorem eu luctus malesuada, velit lectus bibendum sapien, et finibus nulla ipsum eget orci. Donec a augue eu orci sagittis iaculis. Maecenas hendrerit et elit quis tincidunt. Duis non tortor sed libero mattis euismod. Nullam vestibulum tempor enim non rutrum."
"Nulla non velit lorem. Pellentesque lacinia urna suscipit commodo rutrum. Nam a lacus sed urna fringilla imperdiet vitae malesuada sem. Sed at nibh tempus, hendrerit diam ac, vestibulum massa. Duis a leo ut nisl hendrerit fermentum id a dolor. Duis iaculis egestas tristique. Maecenas a nisl in nulla varius convallis vel ac enim. Nulla nec velit mattis, efficitur dolor at, luctus elit. Interdum et malesuada fames ac ante ipsum primis in faucibus.";
    int rw = vfs_write(node, 0, sizeof(buf), buf);
    printf("wrote: %d, expected: %d\n", rw, sizeof(buf));
    
    long s = vfs_ftell(node);
    char *b2 = (char*)kmalloc(s + 1);
    int rr = vfs_read(node, 0, s, b2);
    b2[rr]='\0';
    kprintf("read: %d, expected: %d\nData: {%s}\n", rr, s, b2);
    kfree(b2);
    kfree(node);

    i = 0;
    kprintf("\nRe-iterating /\n");
    while ((dir = vfs_readdir(_RootFS, i++)))
    {
        kprintf("%s %u\n", dir->name, dir->ino);
        kfree(dir);
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