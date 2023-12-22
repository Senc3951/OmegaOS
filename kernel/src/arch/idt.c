#include <arch/idt.h>
#include <libc/string.h>

extern uint64_t interruptHandlers[];

static IDTEntry_t g_idtEntries[IDT_ENTRIES];
static IDT_t g_idt;

static void setEntry(const uint8_t index, const uint64_t offset, const uint16_t segment, const uint8_t flags)
{
    IDTEntry_t *entry = &g_idtEntries[index];

    entry->offsetLow = (uint16_t)(offset & 0xFFFF);
    entry->offsetMid = (uint16_t)((offset >> 16) & 0xFFFF);
    entry->offsetHigh = (uint32_t)((offset >> 32) & 0xFFFFFFFF);
    
    entry->segment = segment;
    entry->flags = flags;
    entry->empty = 0;
    entry->reserved = 0;
}

void idt_load()
{
    memset(g_idtEntries, 0, sizeof(g_idtEntries));
    for (size_t i = 0; i < IDT_ENTRIES; i++)
    {
        if (i >= IRQ0 && i <= IRQ15)
            setEntry(i, interruptHandlers[i], KERNEL_CS, IDT_TRAP_TYPE);
        else
            setEntry(i, interruptHandlers[i], KERNEL_CS, IDT_INTERRUPT_TYPE3);
    }
    
    g_idt.base = (uint64_t)g_idtEntries;
    g_idt.size = sizeof(g_idtEntries) - 1;
    
    asm volatile("lidtq %0" : : "m"(g_idt));
}