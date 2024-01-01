#pragma once

#include <common.h>

typedef struct QUEUE_NODE
{
    void *data;
    struct QUEUE_NODE *next;
} QueueNode_t;

typedef struct QUEUE
{
    QueueNode_t *front, *rear;
    size_t count;
} Queue_t;

Queue_t *queue_create();
void queue_enqueue(Queue_t *q, void *data);
void *queue_deqeueue(Queue_t *q);
void queue_remove(Queue_t *q, void *data);