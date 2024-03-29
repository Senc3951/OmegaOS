#pragma once

#include <common.h>
#include <cpuid.h>

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

/// @brief Read the time-stamp counter.
/// @return Time-Stamp counter.
inline uint64_t __rdtsc()
{
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    
    return ((uint64_t)high << 32) | low;
}