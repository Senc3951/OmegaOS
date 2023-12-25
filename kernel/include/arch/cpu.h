#pragma once

#include <common.h>

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
