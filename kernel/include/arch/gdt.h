#pragma once

#include <common.h>

/// @brief TSS entry.
typedef struct TSS_ENTRY
{
    uint32_t reserved0;
    uint64_t rsp[3];
    uint32_t reserved1[2];
    uint64_t ist[7];
    uint32_t reserved2[2];
    uint16_t reserved3;
    uint16_t ioMapBase;
} __PACKED__ TSSEntry_t;

/// @brief TSS descriptor. 
typedef struct TSS_DESCRIPTOR
{
    uint16_t limitLow;
    uint16_t baseLow;
    uint8_t	baseMid;
    uint8_t	access;
    uint8_t	flags;
    uint8_t	baseMid2;
    uint32_t baseHigh;
    uint32_t reserved;
} __PACKED__ TSSDescriptor_t;

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

#define GDT_KERNEL_CS           0x8
#define GDT_KERNEL_DS           0x10
#define GDT_USER_CS             ((4 * 8) | 0b11)
#define GDT_USER_DS             ((5 * 8) | 0b11)
#define GDT_TSS_INDEX           ((6 * 8) | 0b11)

#define GDT_PRESENT             (1 << 7)
#define GDT_DPL0                (0 << 5)
#define GDT_DPL3                (3 << 5)
#define GDT_TYPE_TSS            (0 << 4)
#define GDT_TYPE_SEGMENT        (1 << 4)
#define GDT_EXEC_DATA_SEGMENT   (0 << 3)
#define GDT_EXEC_CODE_SEGMENT   (1 << 3)
#define GDT_READ_CODE_SEGMENT   (1 << 1)
#define GDT_READ_DATA_SEGMENT   (1 << 1)

#define KERNEL_CODE_SEGMENT     (GDT_PRESENT | GDT_DPL0 | GDT_TYPE_SEGMENT | GDT_EXEC_CODE_SEGMENT | GDT_READ_CODE_SEGMENT)
#define KERNEL_DATA_SEGMENT     (GDT_PRESENT | GDT_DPL0 | GDT_TYPE_SEGMENT | GDT_EXEC_DATA_SEGMENT | GDT_READ_DATA_SEGMENT)
#define USER_CODE_SEGMENT       (GDT_PRESENT | GDT_DPL3 | GDT_TYPE_SEGMENT | GDT_EXEC_CODE_SEGMENT | GDT_READ_CODE_SEGMENT)
#define USER_DATA_SEGMENT       (GDT_PRESENT | GDT_DPL3 | GDT_TYPE_SEGMENT | GDT_EXEC_DATA_SEGMENT | GDT_READ_DATA_SEGMENT)

/// @brief Load the GDT into the CPU.
void gdt_load();

/// @brief Add stack(s) to the tss in case of switch to ring0 / certain interrupts.
void tss_late_set();