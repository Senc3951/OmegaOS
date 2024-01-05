#pragma once

#include <common.h>
#include <cpuid.h>

enum
{
    PAT_UNCACHEABLE     = 0,
    PAT_WRITE_COMBINING = 1,
    PAT_WRITE_THROUGH   = 4,
    PAT_WRITE_PROTECT   = 5,
    PAT_WRITE_BACK      = 6,
    PAT_UNCACHED        = 7
};

/// @brief Read the cr0 register.
#define READ_CR0() ({                               \
    uint64_t cr0;                                   \
    asm volatile("mov %%cr0, %0" : "=r"(cr0));      \
    cr0;                                            \
})

/// @brief Read the cr2 register.
#define READ_CR2() ({                               \
    uint64_t cr2;                                   \
    asm volatile("mov %%cr2, %0" : "=r"(cr2));      \
    cr2;                                            \
})

/// @brief Read the cr3 register.
#define READ_CR3() ({                               \
    uint64_t cr3;                                   \
    asm volatile("mov %%cr3, %0" : "=r"(cr3));      \
    cr3;                                            \
})

/// @brief Read the cr4 register.
#define READ_CR4() ({                               \
    uint64_t cr4;                                   \
    asm volatile("mov %%cr4, %0" : "=r"(cr4));      \
    cr4;                                            \
})

/// @brief Write to the CR0 register.
/// @param cr0 Value to write.
#define WRITE_CR0(cr0) { asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory"); }

/// @brief Write to the CR4 register.
/// @param cr4 Value to write.
#define WRITE_CR4(cr4) { asm volatile("mov %0, %%cr4" :: "r"(cr4) : "memory"); }

/// @brief Read MSR.
/// @param msr msr to read.
/// @param lo low part.
/// @param hi high part.
void __rdmsr(uint32_t msr, uint32_t *lo, uint32_t *hi);

/// @brief Write MSR.
/// @param msr msr to write.
/// @param lo low part.
/// @param hi high part.
void __wrmsr(uint32_t msr, uint32_t lo, uint32_t hi);