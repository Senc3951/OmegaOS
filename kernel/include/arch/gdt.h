#pragma once

#include <common.h>

/// @brief Entry in the GDT.
typedef struct GDT_ENTRY
{
    uint16_t limitLow;
    uint16_t baseLow;
    uint8_t baseMiddle;
    uint8_t access;
    uint8_t granularity;
    uint8_t baseHigh;
} __PACKED__ GDTEntry_t;

/// @brief Struct the CPU will receive.
typedef struct GDT
{
    uint16_t size;
    uint64_t base;
} __PACKED__ GDT_t;

/// @brief Load the GDT into the CPU.
void gdt_load();