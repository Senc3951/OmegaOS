#pragma once

#include <common.h>

/// @brief Initialize the heap.
/// @param start Start of the heap.
/// @param size Size of the heap.
void heap_init(const uint64_t start, const size_t size);

/// @brief Allocate memory.
/// @param size Size in bytes of allocated memory.
/// @return Pointer to the allocated memory, NULL if failed.
void *kmalloc(const size_t size);

/// @brief Allocate and zero memory.
/// @param size Size in bytes of allocated memory.
/// @return Pointer to the allocated memory, NULL if failed.
void *kcalloc(const size_t size);

/// @brief Reallocate memory.
/// @param addr Address of previous memory.
/// @param ns New size of allocate to.
/// @return Pointer to new memory.
void *krealloc(void *addr, const size_t ns);

/// @brief Free previously allocated memory.
/// @param addr Address of allocated memory.
void kfree(void *addr);