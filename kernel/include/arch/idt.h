#pragma once

#include <common.h>

#define IDT_ENTRIES     256

#define IRQ_IST     1
#define PIT_IST     2
#define PS2_KBD_IST 3

#define PIT_ISR     IRQ0
#define PS2_KBD_ISR (PIT_ISR + 1)

/// @brief Entry in the IDT.
typedef struct IDT_ENTRY
{
    uint16_t offsetLow;
    uint16_t segment;
    uint8_t ist : 3;
    uint8_t reserved0 : 5;
    uint8_t flags;
    uint16_t offsetMid;
    uint32_t offsetHigh;
    uint32_t reserved1;
} __PACKED__ IDTEntry_t;

/// @brief Struct the CPU will receive.
typedef struct IDT
{
    uint16_t size;
    uint64_t base;
} __PACKED__ IDT_t;

#define IDT_PRESENT     (1 << 7)
#define IDT_DPL0        (0 << 5)
#define IDT_DPL3        (3 << 5)
#define IDT_INTERRUPT   (0b1110)
#define IDT_TRAP        (0b1111)

#define IDT_INTERRUPT_TYPE0 (IDT_PRESENT | IDT_DPL0 | IDT_INTERRUPT)
#define IDT_INTERRUPT_TYPE3 (IDT_PRESENT | IDT_DPL3 | IDT_INTERRUPT)
#define IDT_TRAP_TYPE0      (IDT_PRESENT | IDT_DPL0 | IDT_TRAP)
#define IDT_TRAP_TYPE3      (IDT_PRESENT | IDT_DPL3 | IDT_TRAP)

/// @brief Load the IDT into the CPU.
void idt_load();