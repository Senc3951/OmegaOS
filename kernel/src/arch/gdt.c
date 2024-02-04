#include <arch/gdt.h>
#include <arch/idt.h>
#include <mem/vmm.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

static GDTBlock_t g_gdtBlock;
static GDT_t g_gdt;
static bool g_bspInitialized = false;

static void setEntry(const uint8_t i, const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t granularity)
{
    GDTEntry_t *entry = &g_gdtBlock.gdt[i];
    
    entry->limitLow = limit & 0xFFFF;
    entry->baseLow = base & 0xFFFF;
    entry->baseMiddle = ((base >> 16) & 0xFF);
    entry->access = access;
    entry->granularity = (((limit >> 16) & 0x0F) | (granularity & 0xF0));
    entry->baseHigh = ((base >> 24) & 0xFF);
}

static void setTSS(const uint8_t i)
{
	TSSEntry_t *tss = (TSSEntry_t *)(&g_gdtBlock.tss);
	*tss = (TSSEntry_t) { .ioMapBase = 0xFFFF };
    
	TSSDescriptor_t* tssDescriptor = (TSSDescriptor_t *)(g_gdtBlock.gdt + i);
	*tssDescriptor = (TSSDescriptor_t)
    {
		.limitLow = sizeof(TSSEntry_t),
		.baseLow = (uint64_t)tss & 0xffff,
		.baseMid = ((uint64_t)tss >> 16) & 0xff,
		.baseMid2 = ((uint64_t)tss >> 24) & 0xff,
		.baseHigh = ((uint64_t)tss >> 32) & 0xffffffff,
		.access = 0b10001001,               // Present, Execute-Only, Accessed
		.flags = 0b00010000					// Available
	};
}

static void setTssInterrupt(uint16_t interrupt, void *stack, uint64_t stackSize)
{
    g_gdtBlock.tss.ist[interrupt - 1] = (uint64_t)stack + stackSize;
}

static void setTssRing(size_t i, void *stack, uint64_t stackSize)
{
    g_gdtBlock.tss.rsp[i] = (uint64_t)stack + stackSize;
}

void tssSetLate()
{
    void *kstack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(kstack);
    setTssRing(0, kstack, KERNEL_STACK_SIZE);
    
    void *irqStack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(irqStack);
    setTssInterrupt(IRQ_IST, irqStack, KERNEL_STACK_SIZE);
    
    void *timerStack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(timerStack);
    setTssInterrupt(PIT_IST, timerStack, KERNEL_STACK_SIZE);
    
    void *ps2Stack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(ps2Stack);
    setTssInterrupt(PS2_KBD_IST, ps2Stack, KERNEL_STACK_SIZE);
    
    LOG("TSS Stacks. Kernel at %p, IRQ at %p, Timer at %p, PS2 kbd at %p\n", kstack, irqStack, timerStack, ps2Stack);
}

void gdt_load()
{
    extern void x64_load_gdt(GDT_t *gdt, uint64_t cs, uint64_t ds);
    extern void x64_flush_tss(uint16_t index);
    
    if (g_bspInitialized)
    {
        x64_load_gdt(&g_gdt, GDT_KERNEL_CS, GDT_KERNEL_DS);
        return;
    }
    
    memset(&g_gdtBlock, 0, sizeof(GDTBlock_t));
    setEntry(0, 0, 0, 0, 0);                                    // Kernel null segment
    setEntry(1, 0, 0xFFFFFFFF, KERNEL_CODE_SEGMENT, 0xAF);      // Kernel code segment
    setEntry(2, 0, 0xFFFFFFFF, KERNEL_DATA_SEGMENT, 0xAF);      // Kernel data segment
    setEntry(4, 0, 0xFFFFFFFF, USER_CODE_SEGMENT, 0xAF);        // User code segment
    setEntry(5, 0, 0xFFFFFFFF, USER_DATA_SEGMENT, 0xAF);        // User data segment
    setTSS(6);                                                  // TSS

    g_gdt.base = (uint64_t)g_gdtBlock.gdt;
    g_gdt.size = sizeof(g_gdtBlock.gdt) - 1;
    
    x64_load_gdt(&g_gdt, GDT_KERNEL_CS, GDT_KERNEL_DS);
    x64_flush_tss(GDT_TSS_INDEX);
    
    tssSetLate();
    g_bspInitialized = true;
}