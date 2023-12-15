#include <mem/heap.h>
#include <mem/vmm.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

// https://wiki.osdev.org/User:Mrvn/LinkedListBucketHeapImplementation

typedef struct DList
{
    struct DList *next;
    struct DList *prev;
} DList_t;

static void dlist_init(DList_t *dlist)
{
    dlist->next = dlist;
    dlist->prev = dlist;
}

static void dlist_insert_after(DList_t *d1, DList_t *d2)
{
    DList_t *n1 = d1->next;
    DList_t *e2 = d2->prev;
 
    d1->next = d2;
    d2->prev = d1;
    e2->next = n1;
    n1->prev = e2;
}

static void dlist_insert_before(DList_t *d1, DList_t *d2)
{
    DList_t *e1 = d1->prev;
    DList_t *e2 = d2->prev;
 
    e1->next = d2;
    d2->prev = e1;
    e2->next = d1;
    d1->prev = e2;
}

static void dlist_remove(DList_t *d)
{
    d->prev->next = d->next;
    d->next->prev = d->prev;
    d->next = d;
    d->prev = d;    
}
 
static void dlist_push(DList_t **d1p, DList_t *d2)
{
	if (*d1p)
	    dlist_insert_before(*d1p, d2);
    
    *d1p = d2;
}

static DList_t *dlist_pop(DList_t **dp)
{
    DList_t *d1 = *dp;
    DList_t *d2 = d1->next;
    dlist_remove(d1);
    if (d1 == d2)
	    *dp = NULL;
    else
	    *dp = d2;
    
    return d1;
}

static void dlist_remove_from(DList_t **d1p, DList_t *d2)
{
    if (*d1p == d2)
	    dlist_pop(d1p);
    else
	    dlist_remove(d2);
}

#define CONTAINER(C, l, v)      ((C *)(((char *)v) - (intptr_t)&(((C *)0)->l)))
#define OFFSETOF(TYPE, MEMBER)  __builtin_offsetof(TYPE, MEMBER)

#define DLIST_INIT(v, l) dlist_init(&v->l)

#define DLIST_REMOVE_FROM(h, d, l)					\
    {									            \
	typeof(**h) **h_ = h, *d_ = d;					\
	DList_t *head = &(*h_)->l;					    \
    dlist_remove_from(&head, &d_->l);				\
	if (head == NULL) {						        \
	    *h_ = NULL;							        \
	} else {							            \
	    *h_ = CONTAINER(typeof(**h), l, head);		\
	}								                \
    }

#define DLIST_PUSH(h, v, l)						\
    {									        \
	typeof(*v) **h_ = h, *v_ = v;				\
	DList_t *head = &(*h_)->l;					\
	if (*h_ == NULL) head = NULL;				\
	dlist_push(&head, &v_->l);					\
	*h_ = CONTAINER(typeof(*v), l, head);		\
    }

#define DLIST_POP(h, l)							\
    ({									        \
	typeof(**h) **h_ = h;						\
	DList_t *head = &(*h_)->l;					\
	DList_t *res = dlist_pop(&head);			\
	if (head == NULL) {						    \
	    *h_ = NULL;							    \
	} else {							        \
	    *h_ = CONTAINER(typeof(**h), l, head);	\
	}								            \
	CONTAINER(typeof(**h), l, res);				\
    })

#define DLIST_ITERATOR_BEGIN(h, l, it)					                \
    {									                                \
        typeof(*h) *h_ = h;						                        \
	DList_t *last_##it = h_->l.prev, *iter_##it = &h_->l, *next_##it;	\
	do {								                                \
	    if (iter_##it == last_##it) {				                    \
		next_##it = NULL;					                            \
	    } else {							                            \
		next_##it = iter_##it->next;				                    \
	    }								                                \
	    typeof(*h)* it = CONTAINER(typeof(*h), l, iter_##it);

#define DLIST_ITERATOR_END(it)						\
	} while((iter_##it = next_##it));				\
    }

#define DLIST_ITERATOR_REMOVE_FROM(h, it, l) DLIST_REMOVE_FROM(h, iter_##it, l)

typedef struct Chunk_t
{
    DList_t all;
    int used;
    union
    {
        char data[0];
        DList_t free;
    };
} Chunk_t;

#define NUM_SIZES       32
#define ALIGN           4
#define MIN_SIZE        sizeof(DList_t)
#define HEADER_SIZE     OFFSETOF(Chunk_t, data)

Chunk_t *g_freeChunks[NUM_SIZES] = { NULL };
Chunk_t *g_first = NULL, *g_last = NULL;

static void memory_chunk_init(Chunk_t *chunk)
{
	DLIST_INIT(chunk, all);
    chunk->used = 0;
    DLIST_INIT(chunk, free);
}

static size_t memory_chunk_size(const Chunk_t *chunk)
{
	char *end = (char *)(chunk->all.next);
    char *start = (char *)(&chunk->all);
    return (end - start) - HEADER_SIZE;
}

static int memory_chunk_slot(size_t size)
{
    int n = -1;
    while(size > 0)
    {
        ++n;
        size /= 2;
    }

    return n;
}

static void remove_free(Chunk_t *chunk)
{
    size_t len = memory_chunk_size(chunk);
    int n = memory_chunk_slot(len);
    DLIST_REMOVE_FROM(&g_freeChunks[n], chunk, free);
}

static void push_free(Chunk_t *chunk)
{
    size_t len = memory_chunk_size(chunk);
    int n = memory_chunk_slot(len);
    DLIST_PUSH(&g_freeChunks[n], chunk, free);
}

void heap_init(const uint64_t start, const size_t size)
{
    char *mem_start = (char *)(((intptr_t)start + ALIGN - 1) & (~(ALIGN - 1)));
    char *mem_end = (char *)(((intptr_t)start + size) & (~(ALIGN - 1)));
    g_first = (Chunk_t *)mem_start;
    Chunk_t *second = g_first + 1;
    g_last = ((Chunk_t *)mem_end) - 1;

    memory_chunk_init(g_first);
    memory_chunk_init(second);
    memory_chunk_init(g_last);
    dlist_insert_after(&g_first->all, &second->all);
    dlist_insert_after(&second->all, &g_last->all);
    g_first->used = 1;
    g_last->used = 1;
 
    size_t len = memory_chunk_size(second);
    int n = memory_chunk_slot(len);
    DLIST_PUSH(&g_freeChunks[n], second, free);
}

void *kmalloc(size_t size)
{
    size = (size + ALIGN - 1) & (~(ALIGN - 1));
	if (size < MIN_SIZE)
        size = MIN_SIZE;
    
	int n = memory_chunk_slot(size - 1) + 1;
	if (n >= NUM_SIZES)
        return NULL;
    
	while(!g_freeChunks[n])
    {
		++n;
		if (n >= NUM_SIZES)
            return NULL;
    }

	Chunk_t *chunk = DLIST_POP(&g_freeChunks[n], free);
    size_t len = 0, size2 = memory_chunk_size(chunk);
    if (size + sizeof(Chunk_t) <= size2) {
		Chunk_t *chunk2 = (Chunk_t*)(((char*)chunk) + HEADER_SIZE + size);
		memory_chunk_init(chunk2);
		dlist_insert_after(&chunk->all, &chunk2->all);

		len = memory_chunk_size(chunk2);
		int n = memory_chunk_slot(len);
		DLIST_PUSH(&g_freeChunks[n], chunk2, free);
    }
    
	chunk->used = 1;
    return chunk->data;
}

void *kcalloc(size_t size)
{
    void *addr = kmalloc(size);
    if (!addr)
        return NULL;
    
    memset(addr, 0, size);
    return addr;
}

void *krealloc(void *addr, size_t ns)
{
    if (!addr)
        return NULL;
    
    Chunk_t *chunk = (Chunk_t*)((char*)addr - HEADER_SIZE);
    size_t chunkSize = memory_chunk_size(chunk);
    
    void *ptr = kmalloc(ns);
    if (!ptr)
        return NULL;
    
    memcpy(ptr, addr, chunkSize);
    kfree(addr);
    return ptr;
}

void kfree(void *addr)
{
    if (!addr)
        return;
    
    Chunk_t *chunk = (Chunk_t *)((char*)addr - HEADER_SIZE);
    assert(chunk->used);
    
    Chunk_t *next = CONTAINER(Chunk_t, all, chunk->all.next);
    Chunk_t *prev = CONTAINER(Chunk_t, all, chunk->all.prev);
    if (next->used == 0)
    {
		remove_free(next);
		dlist_remove(&next->all);
    }
    if (prev->used == 0)
    {
		remove_free(prev);
		dlist_remove(&chunk->all);
		push_free(prev);
    }
    else
    {
		chunk->used = 0;
		DLIST_INIT(chunk, free);
		push_free(chunk);
    }
}