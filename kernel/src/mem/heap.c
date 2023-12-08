#include <mem/heap.h>
#include <assert.h>
#include <libc/string.h>

typedef struct HEAP_CHUNK
{
    struct HEAP_CHUNK *next;
    struct HEAP_CHUNK *prev;
    bool allocated;
    size_t size;
} HeapChunk_t;

static HeapChunk_t *g_head;

void heap_init(const uint64_t start, const size_t size)
{
    assert(size >= sizeof(HeapChunk_t));

    g_head = (HeapChunk_t *)start;
    g_head->prev = g_head->next = NULL;
    g_head->size = size - sizeof(HeapChunk_t);
    g_head->allocated = false;
}

void *kmalloc(const size_t size)
{
    HeapChunk_t *result = NULL;
    for (HeapChunk_t *chunk = g_head; chunk && !result; chunk = chunk->next)
    {
        if (chunk->size > size && !chunk->allocated)
            result = chunk;
    }
    
    if (!result)
        return NULL;
    if (result->size >= size + sizeof(HeapChunk_t) + 1)
    {
        HeapChunk_t* temp = (HeapChunk_t *)((uint64_t)result + sizeof(HeapChunk_t) + size);
        temp->allocated = false;
        temp->size = result->size - size - sizeof(HeapChunk_t);
        temp->prev = result;
        temp->next = result->next;
        if (temp->next)
            temp->next->prev = temp;
        
        result->size = size;
        result->next = temp;
    }
    
    result->allocated = true;
    return (void *)((uint64_t)result + sizeof(HeapChunk_t));
}

void *kcalloc(const size_t size)
{
    void *addr = kmalloc(size);
    if (!addr)
        return NULL;
    
    memset(addr, 0, size);
    return addr;
}

void *krealloc(void *addr, const size_t ns)
{
    if (!addr)
        return NULL;
    
    HeapChunk_t* chunk = (HeapChunk_t *)((uint64_t)addr - sizeof(HeapChunk_t));
    assert(chunk->allocated);
    size_t size = chunk->size;
    kfree(addr);
    
    void *ptr = kmalloc(ns);
    if (!ptr)
        return NULL;
    
    memcpy(ptr, addr, size);
    return ptr;
}

void kfree(void *addr)
{
    if (!addr)
        return;
    
    HeapChunk_t* chunk = (HeapChunk_t *)((uint64_t)addr - sizeof(HeapChunk_t));
    assert(chunk->allocated);
    chunk->allocated = false;
    
    if (chunk->prev && !chunk->prev->allocated)
    {
        chunk->prev->next = chunk->next;
        chunk->prev->size += chunk->size + sizeof(HeapChunk_t);
        if (chunk->next)
            chunk->next->prev = chunk->prev;
        
        chunk = chunk->prev;
    }
    
    if (chunk->next && !chunk->prev->allocated)
    {
        chunk->size += chunk->next->size + sizeof(HeapChunk_t);
        chunk->next = chunk->next->next;
        if (chunk->next)
            chunk->next->prev = chunk;
    }
}