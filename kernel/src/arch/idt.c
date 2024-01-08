#include <arch/idt.h>
#include <arch/gdt.h>
#include <libc/string.h>

extern uint64_t interruptHandlers[];

static IDTEntry_t g_idtEntries[IDT_ENTRIES];
static IDT_t g_idt;
static bool g_initialized = false;

#define LOAD_IDT(idt) { asm volatile("lidtq %0" : : "m"(idt)); }

static void setEntry(const uint8_t index, const uint64_t offset, const uint16_t segment, const uint8_t flags, uint8_t ist)
{
    IDTEntry_t *entry = &g_idtEntries[index];
    
    entry->offsetLow = (uint16_t)(offset & 0xFFFF);
    entry->offsetMid = (uint16_t)((offset >> 16) & 0xFFFF);
    entry->offsetHigh = (uint32_t)((offset >> 32) & 0xFFFFFFFF);
    
    entry->segment = segment;
    entry->flags = flags;
    entry->ist = ist;
    entry->reserved0 = entry->reserved1 = 0;
}

void idt_load()
{
    if (g_initialized)
    {
        LOAD_IDT(g_idt);
        return;
    }

    memset(g_idtEntries, 0, sizeof(g_idtEntries));
    for (size_t i = 0; i < IDT_ENTRIES; i++)
    {
        if (i >= IRQ0 && i <= IRQ15)
            setEntry(i, interruptHandlers[i], GDT_KERNEL_CS, IDT_TRAP_TYPE0, 0);
        else if (i == SYSCALL)
            setEntry(i, interruptHandlers[i], GDT_KERNEL_CS, IDT_INTERRUPT_TYPE3, 0);
        else
            setEntry(i, interruptHandlers[i], GDT_KERNEL_CS, IDT_INTERRUPT_TYPE0, 0);
    }
    
    g_idt.base = (uint64_t)g_idtEntries;
    g_idt.size = sizeof(g_idtEntries) - 1;
    g_initialized = true;
    
    LOAD_IDT(g_idt);
}