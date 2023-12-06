#pragma once

#include <common.h>

extern uint64_t _KernelStart, _KernelEnd;

/// @brief Information about the framebuffer.
typedef struct FRAMEBUFFER
{
    void *baseAddress;
    uint64_t bufferSize;
    uint32_t width;
    uint32_t height;
    uint32_t pixelsPerScanLine;
    uint32_t bytesPerPixel;
} __PACKED__ Framebuffer_t;

/// @brief Header of a PSF1 font file.
typedef struct PSF1_HEADER
{
    uint8_t magic[2];
    uint8_t mode;
    uint8_t charsize;
} __PACKED__ PSF1Header_t;

/// @brief Body of a PSA1 font file.
typedef struct PSF1_FONT
{
    PSF1Header_t *header;
    void* glyphBuffer;
} __PACKED__ PSF1Font_t;

/// @brief Contains information about memory regions.
typedef struct MEMORY_DESCRIPTOR
{
    uint32_t type;
    uint32_t pad;
    uint64_t physicalStart;
    uint64_t virtualStart;
    uint64_t numberOfPages;
    uint64_t attributes;
} __PACKED__ MemoryDescriptor_t;

/// @brief Struct given by the bootloader.
typedef struct BOOT_INFO
{
	Framebuffer_t *fb;
	PSF1Font_t *font;
	MemoryDescriptor_t *mmap;
	uint64_t mmapSize;
	uint64_t mmapDescriptorSize;
} __PACKED__ BootInfo_t;