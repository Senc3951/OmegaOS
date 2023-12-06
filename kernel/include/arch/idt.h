#pragma once

#include <common.h>

#define IDT_ENTRIES         256
#define IDT_INTERRUPT_GATE  0x8E

/// @brief Entry in the IDT.
typedef struct IDT_ENTRY
{
    uint16_t offsetLow;
    uint16_t segment;
    uint8_t empty;
    uint8_t flags;
    uint16_t offsetMid;
    uint32_t offsetHigh;
    uint32_t reserved;
} __PACKED__ IDTEntry_t;

/// @brief Struct the CPU will receive.
typedef struct IDT
{
    uint16_t size;
    uint64_t base;
} __PACKED__ IDT_t;

/// @brief Load the IDT into the CPU.
void idt_load();