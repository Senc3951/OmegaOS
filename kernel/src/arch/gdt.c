#include <arch/gdt.h>

static GDTEntry_t g_gdtEntries[6];
static GDT_t g_gdt;

static void setEntry(const uint8_t i, const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t granularity)
{
    GDTEntry_t *entry = &g_gdtEntries[i];
    
    entry->limitLow = limit & 0xFFFF;
    entry->baseLow = base & 0xFFFF;
    entry->baseMiddle = ((base >> 16) & 0xFF);
    entry->access = access;
    entry->granularity = (((limit >> 16) & 0xF) | (granularity & 0xF0));
    entry->baseHigh = ((base >> 24) & 0xFF);
}

void gdt_load()
{
    setEntry(0, 0, 0, 0, 0);                    // Kernel null segment
    setEntry(1, 0, 0xFFFFFFFF, 0x9A, 0xAF);     // Kernel code segment
    setEntry(2, 0, 0xFFFFFFFF, 0x92, 0xAF);     // Kernel data segment
    setEntry(3, 0, 0, 0, 0);                    // User null segment
    setEntry(4, 0, 0xFFFFFFFF, 0xFA, 0xAF);     // User code segment
    setEntry(5, 0, 0xFFFFFFFF, 0xF2, 0xAF);     // User data segment
    
    g_gdt.base = (uint64_t)g_gdtEntries;
    g_gdt.size = sizeof(g_gdtEntries) - 1;
    
    extern void x64_load_gdt(GDT_t *gdt, uint64_t cs, uint64_t ds);
    x64_load_gdt(&g_gdt, KERNEL_CS, KERNEL_DS);
}