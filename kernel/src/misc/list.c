#include <misc/list.h>
#include <mem/heap.h>

void list_destroy(list_t *list)
{
	node_t *n = list->head;
	while (n)
	{
		kfree(n->value);
		n = n->next;
	}
}

void list_free(list_t *list)
{
	node_t *n = list->head;
	while (n)
	{
		node_t * s = n->next;
		kfree(n);
		
		n = s;
	}
}

void list_append(list_t *list, node_t *node)
{
	node->next = NULL;
	if (!list->tail)
	{
		list->head = node;
		node->prev = NULL;
	}
	else
	{
		list->tail->next = node;
		node->prev = list->tail;
	}

	list->tail = node;
	list->length++;
}

int list_insert(list_t *list, void *item)
{
	node_t *node = (node_t *)kmalloc(sizeof(node_t));
	if (!node)
		return -1;
	
	node->value = item;
	node->next  = NULL;
	node->prev  = NULL;
	list_append(list, node);
	
	return list->length - 1;
}

void list_append_after(list_t *list, node_t *before, node_t *node)
{
	if (!list->tail)
	{
		list_append(list, node);
		return;
	}

	if (!before)
	{
		node->next = list->head;
		list->head->prev = node;
		list->head = node;
		list->length++;

		return;
	}
	if (before == list->tail)
		list->tail = node;
	else
	{
		before->next->prev = node;
		node->next = before->next;
	}

	node->prev = before;
	before->next = node;
	list->length++;
}

void list_insert_after(list_t *list, node_t *before, void *item)
{
	node_t *node = (node_t *)kmalloc(sizeof(node_t));
	node->value = item;
	node->next  = NULL;
	node->prev  = NULL;
	list_append_after(list, before, node);
}

list_t *list_create()
{
	list_t *out = (list_t *)kmalloc(sizeof(list_t));
	out->head = NULL;
	out->tail = NULL;
	out->length = 0;

	return out;
}

node_t *list_find(list_t *list, void *value)
{
	foreach(item, list) {
		if (item->value == value)
			return item;
	}

	return NULL;
}

node_t *list_find_index(list_t *list, size_t index)
{
	if (index > list->length)
		return NULL;

	size_t i = 0;
	node_t *n = list->head;
	while (i < index)
	{
		n = n->next;
		i++;
	}

	return n;
}

void list_remove(list_t *list, size_t index)
{
	if (index > list->length)
		return;
	
	size_t i = 0;
	node_t * n = list->head;
	while (i < index)
	{
		n = n->next;
		i++;
	}

	list_delete(list, n);
}

void list_delete(list_t *list, node_t *node)
{
	if (node == list->head)
		list->head = node->next;
	if (node == list->tail)
		list->tail = node->prev;
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	
	node->prev = NULL;
	node->next = NULL;
	list->length--;
}

node_t *list_pop(list_t *list)
{
	if (!list->tail)
		return NULL;
	
	node_t *out = list->tail;
	list_delete(list, list->tail);
	return out;
}

node_t *list_dequeue(list_t *list)
{
	if (!list->head)
		return NULL;
	
	node_t *out = list->head;
	list_delete(list, list->head);
	return out;
}

list_t *list_copy(list_t *original)
{
	list_t *out = list_create();
	node_t *node = original->head;
	while (node)
		list_insert(out, node->value);
	
	return out;
}

void list_merge(list_t *target, list_t *source)
{
	if (target->tail)
		target->tail->next = source->head;
	else
		target->head = source->head;
	if (source->tail)
		target->tail = source->tail;
	
	target->length += source->length;
	kfree(source);
}