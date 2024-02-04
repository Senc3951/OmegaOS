#include <mem/pmm.h>
#include <arch/lock.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define INVALID_FRAME_INDEX UINT64_MAX

static MemoryDescriptor_t *g_mmap;
static uint64_t g_mmapSize, g_mmapDescriptorSize;
static uint64_t g_bitmapSize, g_bitmapElementSize, g_lastFoundBitmap;
static uint64_t *g_bitmap;
MAKE_SPINLOCK(g_lock);

static void setFrame(const uint64_t index)
{
    uint64_t pIndex = index / g_bitmapElementSize;
    uint64_t pBit = index % g_bitmapElementSize;
    
    g_bitmap[pIndex] |= (1ULL << pBit);
}

static void clearFrame(const uint64_t index)
{
    uint64_t pIndex = index / g_bitmapElementSize;
    uint64_t pBit = index % g_bitmapElementSize;
    
    g_bitmap[pIndex] &= ~(1ULL << pBit);
}

static bool testFrame(const uint64_t i, const uint8_t bit)
{
    return g_bitmap[i] & (1ULL << bit);
}

static uint64_t getFrames(const size_t frames)
{
    lock_acquire(&g_lock);

    bool fbFound = false;
    uint64_t j, index = 0, bCopy = frames;
    for (uint64_t i = g_lastFoundBitmap; i < g_bitmapSize; i++)
    {
        // Block is not completely used
        if (g_bitmap[i] != UINT64_MAX)
        {
            // Search each bit
            for (j = 0; j < g_bitmapElementSize; j++)
            {
                if (!testFrame(i, j))
                {
                    // If found the first block
                    if (!fbFound)
                    {
                        fbFound = true;
                        index = i * g_bitmapElementSize + j;
                    }
                    
                    bCopy--;
                    if (bCopy == 0)
                    {
                        g_lastFoundBitmap = i;
                        goto found;
                    }
                }
                else    // Block is used
                {
                    if (fbFound)    // Already found a couple of blocks
                    {
                        // Reset the data and continue looking
                        fbFound = false;
                        bCopy = frames;
                    }
                }
            }
        }
    }

    lock_release(&g_lock);
    if (g_lastFoundBitmap == 0)
        return INVALID_FRAME_INDEX;
    
    g_lastFoundBitmap = 0;
    return getFrames(frames);
found:
    lock_release(&g_lock);
    return index;
}

void pmm_init(MemoryDescriptor_t *mmap, const uint64_t mmapSize, const uint64_t mmapDescriptorSize, const Framebuffer_t *fb)
{
    g_mmap = mmap;
    g_mmapSize = mmapSize;
    g_mmapDescriptorSize = mmapDescriptorSize; 
    g_bitmapElementSize = sizeof(*g_bitmap) * 8;
    g_lastFoundBitmap = 0;
    
    uint64_t largestMemSegment = 0;
    MemoryDescriptor_t *largestSegment = NULL;

    // Find largest region to store the bitmap at
    for (uint64_t i = 0; i < g_mmapSize / g_mmapDescriptorSize; i++)
    {
        MemoryDescriptor_t *desc = (MemoryDescriptor_t *)((uint64_t)g_mmap + i * g_mmapDescriptorSize);
        if (desc->type == MMAP_FREE && desc->numberOfPages > largestMemSegment)
        {
            largestMemSegment = desc->numberOfPages;
            largestSegment = desc;
            
            LOG("Physical memory region at %p - %p\n", desc->physicalStart, desc->physicalStart + desc->numberOfPages * PAGE_SIZE);
        }
    }

    // Verify a segment was found
    assert(largestSegment && largestMemSegment != 0);
    
    // Initialize the bitmap
    g_bitmapSize = pmm_getMemorySize() / PAGE_SIZE / g_bitmapElementSize;
    g_bitmap = (uint64_t *)largestSegment->physicalStart;
    uint64_t bitmapSizeInBytes = g_bitmapSize * sizeof(*g_bitmap);
    assert(bitmapSizeInBytes < largestMemSegment);  // Segment must be large enough to hold the bitmap
    LOG("Bitmap at %p (%llu bytes)\n", g_bitmap, bitmapSizeInBytes);
    
    // Reserve entire memory
    memset(g_bitmap, 0xFF, bitmapSizeInBytes);

    // Free usable regions
    for (uint64_t i = 0; i < g_mmapSize / g_mmapDescriptorSize; i++)
    {
        MemoryDescriptor_t *desc = (MemoryDescriptor_t *)((uint64_t)g_mmap + i * g_mmapDescriptorSize);
        if (desc->type == MMAP_FREE)
            pmm_unreserveRegion(desc->physicalStart, desc->numberOfPages);
    }

    // Reserve bitmap
    pmm_reserveRegion((uint64_t)g_bitmap, bitmapSizeInBytes / PAGE_SIZE);

    // Reserve framebuffer
    pmm_reserveRegion((uint64_t)fb->baseAddress, fb->bufferSize / PAGE_SIZE);
    
    // Reserve kernel
    pmm_reserveRegion(0, _KernelEnd / PAGE_SIZE + 1);
}

void *pmm_getFrame()
{
    return pmm_getFrames(1);
}

void *pmm_getFrames(const size_t count)
{
    if (count == 0)
        return NULL;
    
    uint64_t index = getFrames(count);
    if (index == INVALID_FRAME_INDEX)
        return NULL;
    for (size_t i = 0; i < count; i++)
        setFrame(index + i);
            
    return (void *)(index * PAGE_SIZE);
}

void pmm_releaseFrame(void *addr)
{
    pmm_releaseFrames(addr, 1);
}

void pmm_releaseFrames(void *addr, const size_t count)
{
    uint64_t index = (uint64_t)addr;
    assert(index % PAGE_SIZE == 0 && index / PAGE_SIZE <= g_bitmapSize);

    g_lastFoundBitmap = index / PAGE_SIZE;
    for (size_t i = 0; i < count; i++)
        clearFrame(g_lastFoundBitmap + i);
}

void pmm_reserveRegion(uint64_t start, const size_t pc)
{
    uint64_t roundedStart = RNDWN(start, PAGE_SIZE);
    for (size_t i = 0; i < pc; i++)
        setFrame((roundedStart + i * PAGE_SIZE) / PAGE_SIZE);
}

void pmm_unreserveRegion(uint64_t start, const size_t pc)
{
    uint64_t roundedStart = RNDUP(start, PAGE_SIZE);
    for (size_t i = 0; i < pc; i++)
        clearFrame((roundedStart + i * PAGE_SIZE) / PAGE_SIZE);
}

uint64_t pmm_getMemorySize()
{
    static uint64_t memorySize = 0;
    if (memorySize > 0)
        return memorySize;
            
    for (uint64_t i = 0; i < g_mmapSize / g_mmapDescriptorSize; i++)
    {
        MemoryDescriptor_t *desc = (MemoryDescriptor_t *)((uint64_t)g_mmap + i * g_mmapDescriptorSize);
        memorySize += desc->numberOfPages * PAGE_SIZE;
    }
    
    return memorySize;
}