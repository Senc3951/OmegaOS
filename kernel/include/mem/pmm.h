#pragma once

#include <bootinfo.h>

/// @brief Initialize the physical memory manager.
/// @param mmap Memory map.
/// @param mmapSize Memory map size.
/// @param mmapDescriptorSize Descriptor size.
/// @param fb Framebuffer.
void pmm_init(MemoryDescriptor_t *mmap, const uint64_t mmapSize, const uint64_t mmapDescriptorSize, const Framebuffer_t *fb);

/// @brief Allocate a physical page.
/// @return Address of the page, NULL if failed.
void *pmm_getFrame();

/// @brief Allocate contagious physical pages.
/// @param count Amount of pages.
/// @return Address of the pages, NULL if failed.
void *pmm_getFrames(const size_t count);

/// @brief Free a page.
/// @param addr Address of the page to free.
void pmm_releaseFrame(void *addr);

/// @brief Free contagious pages.
/// @param addr Address of the first page.
/// @param count Amount of pages to free.
void pmm_releaseFrames(void *addr, const size_t count);

/// @brief Reserve a region in memory.
/// @param start Start of the region.
/// @param pc Amount of pages.
void pmm_reserveRegion(uint64_t start, const size_t pc);

/// @brief Unreserve a region in memory.
/// @param start Start of the region.
/// @param pc Amount of pages.
void pmm_unreserveRegion(uint64_t start, const size_t pc);

/// @brief Calculate memory size.
/// @return Memory size in bytes.
uint64_t pmm_getMemorySize();