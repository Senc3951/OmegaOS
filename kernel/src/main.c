#include <io/serial.h>
#include <io/io.h>
#include <logger.h>
#include <assert.h>
#include <panic.h>
#include <gui/screen.h>
#include <arch/lock.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/pic.h>
#include <arch/rsdp.h>
#include <arch/apic/madt.h>
#include <arch/apic/apic.h>
#include <arch/apic/ioapic.h>
#include <arch/smp.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/heap.h>
#include <dev/pit.h>
#include <dev/ps2/kbd.h>
#include <dev/timer.h>
#include <dev/storage/ide.h>
#include <fs/ext2.h>
#include <syscall/syscalls.h>
#include <sys/process.h>
#include <sys/scheduler.h>

extern uint64_t _kernel_start, _kernel_end, _kernel_writable_start, _kernel_writable_end;
uint64_t _KernelStart, _KernelEnd, _KernelWritableStart, _KernelWritableEnd;

MAKE_SPINLOCK(g_coreLock);

void dev_init()
{
    // Keyboard
    assert(ps2_kbd_init());
    
    // Timers
    lapic_timer_init();
    if (!_ApicInitialized)
        pit_init(PIT_DEFAULT_FREQUENCY);
}

void bsp_init()
{
    CoreContext_t *bsp = &_Cores[0];
    bsp->id = apic_get_id();
    bsp->currentProcess = NULL;

    void *kstack = vmm_createIdentityPages(_KernelPML4, CORE_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(kstack);
    bsp->stack = (uint64_t)kstack + CORE_STACK_SIZE;
}

int _entry(BootInfo_t *bootInfo)
{
    __CLI();
    eassert(bootInfo && bootInfo->fb && bootInfo->font && bootInfo->mmap && bootInfo->rsdp);
    eassert(serial_init());
    
    core_init(NULL);

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
    pic_init(IRQ0, IRQ0 + 8, false);
    
    // Initialize APIC & I/O APIC
    rsdp_init(bootInfo->rsdp);
    madt_init();
    apic_init();
    ioapic_init();
    
    // Initialize devices
    dev_init();
    __STI();
    
    // Initialize other cores
    lock_acquire(&g_coreLock);
    bsp_init();
    smp_init();

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
    syscalls_init();
    Process_t *idle = process_init();
    scheduler_init();
    
    extern void shell();
    assert(process_create(idle, "Shell", shell, PriorityInteractive));
    
    LOG("Kernel initialization finished. Jumping to user space\n\n");
    lock_release(&g_coreLock);  // Allow other cores to start scheduling
    
    lapic_timer_periodic(1);
    yield(NULL);
    
    panic("Unreachable");
}

int ap_entry(CoreContext_t *context)
{
    LOG("[Core %u] Online\n", context->id);
    gdt_load();
    idt_load();
    apic_set_registers();
    LOG("[Core %u] Initialized\n", context->id);
    
    // For for bsp to finish initialization
    while (lock_used(&g_coreLock))
        __PAUSE();
    
    __HCF();
}