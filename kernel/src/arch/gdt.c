#include <arch/gdt.h>
#include <arch/idt.h>
#include <libc/string.h>
#include <mem/vmm.h>
#include <assert.h>
#include <logger.h>

typedef struct GDT_BLOCK
{
    GDTEntry_t gdt[8];
    TSSEntry_t tss;
} GDTBlock_t;

static GDTBlock_t g_gdtBlock;
static GDT_t g_gdt;

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

static void setTssInterrupt(uint16_t interrupt, void *stack)
{
    g_gdtBlock.tss.ist[interrupt - 1] = (uint64_t)stack;
}

static void setTssRing(size_t i, void *stack)
{
    g_gdtBlock.tss.rsp[i] = (uint64_t)stack;
}

void tssSetLate()
{
    void *kstack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(kstack);
    setTssRing(0, (void *)((uint64_t)kstack + KERNEL_STACK_SIZE));
    
    void *irqStack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(irqStack);
    setTssInterrupt(1, (void *)((uint64_t)irqStack + KERNEL_STACK_SIZE));
    
    void *pitStack = vmm_createIdentityPages(_KernelPML4, KERNEL_STACK_SIZE / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
    assert(pitStack);
    setTssInterrupt(2, (void *)((uint64_t)pitStack + KERNEL_STACK_SIZE));
    
    LOG("Kernel stack at %p. irq stack at %p. pit stack at %p\n", kstack, irqStack, pitStack);
}

void gdt_load()
{
    memset(g_gdtBlock.gdt, 0, sizeof(g_gdtBlock.gdt));
    memset(&g_gdtBlock.tss, 0, sizeof(g_gdtBlock.tss));
    
    setEntry(0, 0, 0, 0, 0);                                    // Kernel null segment
    setEntry(1, 0, 0xFFFFFFFF, KERNEL_CODE_SEGMENT, 0xAF);      // Kernel code segment
    setEntry(2, 0, 0xFFFFFFFF, KERNEL_DATA_SEGMENT, 0xAF);      // Kernel data segment
    setEntry(4, 0, 0xFFFFFFFF, USER_CODE_SEGMENT, 0xAF);        // User code segment
    setEntry(5, 0, 0xFFFFFFFF, USER_DATA_SEGMENT, 0xAF);        // User data segment
    setTSS(6);                                                  // TSS
        
    g_gdt.base = (uint64_t)g_gdtBlock.gdt;
    g_gdt.size = sizeof(g_gdtBlock.gdt) - 1;
    
    extern void x64_load_gdt(GDT_t *gdt, uint64_t cs, uint64_t ds);
    extern void x64_flush_tss(uint16_t index);
    
    x64_load_gdt(&g_gdt, GDT_KERNEL_CS, GDT_KERNEL_DS);
    x64_flush_tss(GDT_TSS_INDEX);

    tssSetLate();
}