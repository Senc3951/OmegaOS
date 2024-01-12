#pragma once

#include <common.h>

#define KERNEL_HEAP_START       0xFFFFFFFF80000000ULL
#define KERNEL_HEAP_SIZE        (1024 * PAGE_SIZE)
#define KERNEL_HEAP_END         (KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/// @brief Initialize the heap.
void heap_init();

/// @brief Allocate memory.
/// @param size Size in bytes of allocated memory.
/// @return Pointer to the allocated memory, NULL if failed.
__MALLOC__ void *kmalloc(size_t size);

/// @brief Allocate and zero memory.
/// @param size Size in bytes of allocated memory.
/// @return Pointer to the allocated memory, NULL if failed.
__MALLOC__ void *kcalloc(size_t size);

/// @brief Reallocate memory.
/// @param ptr Address of previous memory.
/// @param size New size of allocate to.
/// @return Pointer to new memory.
__MALLOC__ void *krealloc(void *ptr, size_t size);

/// @brief Free previously allocated memory.
/// @param ptr Address of allocated memory.
void kfree(void *ptr);