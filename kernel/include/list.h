#pragma once

#include <common.h>

typedef struct node
{
	struct node *next;
	struct node *prev;
	void *value;
} __PACKED__ node_t;

typedef struct LIST
{
	node_t *head;
	node_t *tail;
	size_t length;
} __PACKED__ list_t;

void list_destroy(list_t *list);
void list_free(list_t *list);
void list_append(list_t *list, node_t *item);
void list_insert(list_t *list, void *item);
list_t *list_create();
node_t *list_find(list_t *list, void *value);
void list_remove(list_t *list, size_t index);
void list_delete(list_t *list, node_t *node);
node_t *list_pop(list_t *list);
node_t *list_dequeue(list_t *list);
list_t *list_copy(list_t *original);
void list_merge(list_t *target, list_t *source);

void list_append_after(list_t *list, node_t *before, node_t *node);
void list_insert_after(list_t *list, node_t *before, void *item);

#define foreach(i, list) for (node_t *i = list->head; i; i = i->next)