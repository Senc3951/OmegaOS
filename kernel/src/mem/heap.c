#include <mem/heap.h>
#include <mem/vmm.h>
#include <arch/spinlock.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define NUM_BINS			11
#define SMALLEST_BIN_LOG	2
#define BIG_BIN				(NUM_BINS - 1)
#define SMALLEST_BIN		(1ULL << SMALLEST_BIN_LOG)
#define PAGE_MASK			(PAGE_SIZE - 1)
#define SKIP_P				INT32_MAX
#define SKIP_MAX_LEVEL		6
#define BIN_MAGIC			0xF92C9DAB

typedef struct _klmalloc_bin_header
{
	struct _klmalloc_bin_header *next;
	void *head;
	uintptr_t size;
	uint32_t magic;
} klmalloc_bin_header_t;

typedef struct _klmalloc_big_bin_header
{
	struct _klmalloc_big_bin_header *next;
	void *head;
	uintptr_t size;
	uint32_t magic;
	struct _klmalloc_big_bin_header *prev;
	struct _klmalloc_big_bin_header *forward[SKIP_MAX_LEVEL + 1];
} klmalloc_big_bin_header_t;

typedef struct _klmalloc_bin_header_head
{
	klmalloc_bin_header_t *first;
} klmalloc_bin_header_head_t;

static struct _klmalloc_big_bins
{
	klmalloc_big_bin_header_t head;
	int level;
} klmalloc_big_bins_t;

static klmalloc_bin_header_head_t g_klmalloc_bin_head[NUM_BINS - 1];
static klmalloc_big_bin_header_t * g_klmalloc_newest_big = NULL;
static uint64_t g_heapEnd = 0;

MAKE_SPINLOCK(g_lock);

static uintptr_t __PURE__ klmalloc_adjust_bin(uintptr_t bin)
{
	if (bin <= (uintptr_t)SMALLEST_BIN_LOG)
		return 0;
	
	bin -= SMALLEST_BIN_LOG + 1;
	if (bin > (uintptr_t)BIG_BIN)
		return BIG_BIN;
	
	return bin;
}

static uintptr_t __PURE__ klmalloc_bin_size(uintptr_t size)
{
	uintptr_t bin = sizeof(size) * 8 - __builtin_clzl(size);
	bin += !!(size & (size - 1));
	
	return klmalloc_adjust_bin(bin);
}

static void *expandHeap(uint32_t size)
{
	if (size % PAGE_SIZE != 0)
		return NULL;
	if (g_heapEnd + size >= KERNEL_HEAP_END)
		return NULL;
	
    void *ret = (void *)g_heapEnd;
    g_heapEnd += size;

    return ret;
}

static void klmalloc_list_decouple(klmalloc_bin_header_head_t *head, klmalloc_bin_header_t *node)
{
	klmalloc_bin_header_t *next	= node->next;
	head->first = next;
	node->next = NULL;
}

static void klmalloc_list_insert(klmalloc_bin_header_head_t *head, klmalloc_bin_header_t *node)
{
	node->next = head->first;
	head->first = node;
}

static klmalloc_bin_header_t *klmalloc_list_head(klmalloc_bin_header_head_t *head)
{
	return head->first;
}

static uint32_t __PURE__ klmalloc_skip_rand()
{
	static uint32_t x = 123456789;
	static uint32_t y = 362436069;
	static uint32_t z = 521288629;
	static uint32_t w = 88675123;

	uint32_t t;
	t = x ^ (x << 11);
	x = y; y = z; z = w;
	
	return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

static int __PURE__ klmalloc_random_level()
{
	int level = 0;
	while (klmalloc_skip_rand() < SKIP_P && level < SKIP_MAX_LEVEL)
		++level;
	
	return level;
}

static klmalloc_big_bin_header_t *klmalloc_skip_list_findbest(uintptr_t search_size)
{
	klmalloc_big_bin_header_t * node = &klmalloc_big_bins_t.head;
	int i;
	for (i = klmalloc_big_bins_t.level; i >= 0; --i)
	{
		while (node->forward[i] && (node->forward[i]->size < search_size))
		{
			node = node->forward[i];
			if (node)
				assert((node->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
		}
	}

	node = node->forward[0];
	if (node)
	{
		assert((uintptr_t)node % PAGE_SIZE == 0);
		assert((node->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
	}

	return node;
}

static void klmalloc_skip_list_insert(klmalloc_big_bin_header_t *value)
{
	assert(value != NULL);
	assert(value->head != NULL);
	assert((uintptr_t)value->head > (uintptr_t)value);
	
	if (value->size > NUM_BINS)
	{
		assert((uintptr_t)value->head < (uintptr_t)value + value->size);
	}
	else
		assert((uintptr_t)value->head < (uintptr_t)value + PAGE_SIZE);
	
	assert((uintptr_t)value % PAGE_SIZE == 0);
	assert((value->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
	assert(value->size != 0);

	klmalloc_big_bin_header_t * node = &klmalloc_big_bins_t.head;
	klmalloc_big_bin_header_t * update[SKIP_MAX_LEVEL + 1];

	int i;
	for (i = klmalloc_big_bins_t.level; i >= 0; --i)
	{
		while (node->forward[i] && node->forward[i]->size < value->size)
		{
			node = node->forward[i];
			if (node)
				assert((node->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
		}

		update[i] = node;
	}

	node = node->forward[0];
	if (node != value)
	{
		int level = klmalloc_random_level();
		if (level > klmalloc_big_bins_t.level)
		{
			for (i = klmalloc_big_bins_t.level + 1; i <= level; ++i)
				update[i] = &klmalloc_big_bins_t.head;
			
			klmalloc_big_bins_t.level = level;
		}

		node = value;
		for (i = 0; i <= level; ++i)
		{
			node->forward[i] = update[i]->forward[i];
			if (node->forward[i])
				assert((node->forward[i]->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
			
			update[i]->forward[i] = node;
		}
	}
}

static void klmalloc_skip_list_delete(klmalloc_big_bin_header_t *value)
{
	assert(value != NULL);
	assert(value->head);
	assert((uintptr_t)value->head > (uintptr_t)value);

	if (value->size > NUM_BINS)
	{
		assert((uintptr_t)value->head < (uintptr_t)value + value->size);
	}
	else
		assert((uintptr_t)value->head < (uintptr_t)value + PAGE_SIZE);
	
	klmalloc_big_bin_header_t *node = &klmalloc_big_bins_t.head;
	klmalloc_big_bin_header_t *update[SKIP_MAX_LEVEL + 1];

	int i;
	for (i = klmalloc_big_bins_t.level; i >= 0; --i)
	{
		while (node->forward[i] && node->forward[i]->size < value->size)
		{
			node = node->forward[i];
			if (node)
				assert((node->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
		}

		update[i] = node;
	}

	node = node->forward[0];
	while (node != value)
		node = node->forward[0];

	if (node != value)
	{
		node = klmalloc_big_bins_t.head.forward[0];
		while (node->forward[0] && node->forward[0] != value)
			node = node->forward[0];
		
		node = node->forward[0];
	}
	
	if (node == value)
	{
		for (i = 0; i <= klmalloc_big_bins_t.level; ++i)
		{
			if (update[i]->forward[i] != node)
				break;
			
			update[i]->forward[i] = node->forward[i];
			if (update[i]->forward[i])
			{
				assert((uintptr_t)(update[i]->forward[i]) % PAGE_SIZE == 0);
				assert((update[i]->forward[i]->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
			}
		}

		while (klmalloc_big_bins_t.level > 0 && klmalloc_big_bins_t.head.forward[klmalloc_big_bins_t.level] == NULL)
			--klmalloc_big_bins_t.level;
	}
}

static void *klmalloc_stack_pop(klmalloc_bin_header_t *header)
{
	assert(header);
	assert(header->head != NULL);
	assert((uintptr_t)header->head > (uintptr_t)header);

	if (header->size > NUM_BINS)
	{
		assert((uintptr_t)header->head < (uintptr_t)header + header->size);
	}
	else
	{
		assert((uintptr_t)header->head < (uintptr_t)header + PAGE_SIZE);
		assert((uintptr_t)header->head > (uintptr_t)header + sizeof(klmalloc_bin_header_t) - 1);
	}
	
	void *item = header->head;
	uintptr_t **head = header->head;
	uintptr_t *next = *head;
	header->head = next;
	
	return item;
}

static void klmalloc_stack_push(klmalloc_bin_header_t *header, void *ptr)
{
	assert(ptr != NULL);
	assert((uintptr_t)ptr > (uintptr_t)header);
	
	if (header->size > NUM_BINS) {
		assert((uintptr_t)ptr < (uintptr_t)header + header->size);
	} else {
		assert((uintptr_t)ptr < (uintptr_t)header + PAGE_SIZE);
	}

	uintptr_t **item = (uintptr_t **)ptr;
	*item = (uintptr_t *)header->head;
	header->head = item;
}

static int klmalloc_stack_empty(klmalloc_bin_header_t *header)
{
	return header->head == NULL;
}

void heap_init()
{
    g_heapEnd = (KERNEL_HEAP_START + PAGE_SIZE) & ~0xFFF;
    vmm_createPages(_KernelPML4, (void *)KERNEL_HEAP_START, (KERNEL_HEAP_END - KERNEL_HEAP_START) / PAGE_SIZE, VMM_KERNEL_ATTRIBUTES);
	
	LOG("Kernel heap: %p - %p\n", KERNEL_HEAP_START, KERNEL_HEAP_END);
}

__MALLOC__ void *kmalloc(size_t size)
{
    if (!size)
		return NULL;
	
	spinlock_acquire(&g_lock);
    uint32_t bucket_id = klmalloc_bin_size(size);
	if (bucket_id < BIG_BIN)
    {
		klmalloc_bin_header_t *bin_header = klmalloc_list_head(&g_klmalloc_bin_head[bucket_id]);
		if (!bin_header)
		{
			bin_header = (klmalloc_bin_header_t*)expandHeap(PAGE_SIZE);
			bin_header->magic = BIN_MAGIC;
			assert((uintptr_t)bin_header % PAGE_SIZE == 0);

			bin_header->head = (void*)((uintptr_t)bin_header + sizeof(klmalloc_bin_header_t));
			klmalloc_list_insert(&g_klmalloc_bin_head[bucket_id], bin_header);
			uintptr_t adj = SMALLEST_BIN_LOG + bucket_id;
			uintptr_t i, available = ((PAGE_SIZE - sizeof(klmalloc_bin_header_t)) >> adj) - 1;

			uintptr_t **base = bin_header->head;
			for (i = 0; i < available; ++i)
				base[i << bucket_id] = (uintptr_t *)&base[(i + 1) << bucket_id];
			
			base[available << bucket_id] = NULL;
			bin_header->size = bucket_id;
		}

		uintptr_t **item = klmalloc_stack_pop(bin_header);
		if (klmalloc_stack_empty(bin_header))
			klmalloc_list_decouple(&(g_klmalloc_bin_head[bucket_id]),bin_header);
		
		spinlock_release(&g_lock);
		return item;
	} else {
		klmalloc_big_bin_header_t *bin_header = klmalloc_skip_list_findbest(size);
		if (bin_header)
		{
			assert(bin_header->size >= size);
			klmalloc_skip_list_delete(bin_header);
			uintptr_t **item = klmalloc_stack_pop((klmalloc_bin_header_t *)bin_header);
			
			spinlock_release(&g_lock);
			return item;
		} else {
			uintptr_t pages = (size + sizeof(klmalloc_big_bin_header_t)) / PAGE_SIZE + 1;
			bin_header = (klmalloc_big_bin_header_t*)expandHeap(PAGE_SIZE * pages);
			bin_header->magic = BIN_MAGIC;
			assert((uintptr_t)bin_header % PAGE_SIZE == 0);
			
			bin_header->size = pages * PAGE_SIZE - sizeof(klmalloc_big_bin_header_t);
			assert((bin_header->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
			bin_header->prev = g_klmalloc_newest_big;
			if (bin_header->prev)
				bin_header->prev->next = bin_header;
			
			g_klmalloc_newest_big = bin_header;
			bin_header->next = NULL;
			bin_header->head = NULL;

			spinlock_release(&g_lock);
			return (void *)((uintptr_t)bin_header + sizeof(klmalloc_big_bin_header_t));
		}
	}
}

__MALLOC__ void *kcalloc(size_t size)
{
    void *addr = kmalloc(size);
    if (!addr)
        return NULL;
    
    memset(addr, 0, size);
    return addr;
}

__MALLOC__ void *krealloc(void *ptr, size_t size)
{
	if (!ptr)
		return kmalloc(size);
	if (!size)
    	return NULL;

	klmalloc_bin_header_t *header_old = (void *)((uintptr_t)ptr & (uintptr_t)~PAGE_MASK);
	assert(header_old->magic == BIN_MAGIC);

	uintptr_t old_size = header_old->size;
	if (old_size < (uintptr_t)BIG_BIN)
		old_size = (1UL << (SMALLEST_BIN_LOG + old_size));
	
	if (old_size >= size)
		return ptr;
	
	void *newptr = kmalloc(size);
	if (newptr)
	{
		memcpy(newptr, ptr, old_size);
		kfree(ptr);

		return newptr;
	}
	
	return NULL;
}

void kfree(void *ptr)
{
	if (__builtin_expect(ptr == NULL, 0))
		return;

	spinlock_acquire(&g_lock);
	if ((uintptr_t)ptr % PAGE_SIZE == 0)
		ptr = (void *)((uintptr_t)ptr - 1);

	klmalloc_bin_header_t * header = (klmalloc_bin_header_t *)((uintptr_t)ptr & (uintptr_t)~PAGE_MASK);
	assert((uintptr_t)header % PAGE_SIZE == 0);
	assert(header->magic == BIN_MAGIC);

	uintptr_t bucket_id = header->size;
	if (bucket_id > (uintptr_t)NUM_BINS)
	{
		bucket_id = BIG_BIN;
		klmalloc_big_bin_header_t *bheader = (klmalloc_big_bin_header_t*)header;
		
		assert(bheader);
		assert(bheader->head == NULL);
		assert((bheader->size + sizeof(klmalloc_big_bin_header_t)) % PAGE_SIZE == 0);
	
		klmalloc_stack_push((klmalloc_bin_header_t *)bheader, (void *)((uintptr_t)bheader + sizeof(klmalloc_big_bin_header_t)));
		assert(bheader->head != NULL);
		klmalloc_skip_list_insert(bheader);
	} else {
		if (klmalloc_stack_empty(header))
			klmalloc_list_insert(&g_klmalloc_bin_head[bucket_id], header);
		
		klmalloc_stack_push(header, ptr);
	}

	spinlock_release(&g_lock);
}