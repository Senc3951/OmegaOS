#include <misc/queue.h>
#include <mem/heap.h>

Queue_t *queue_create()
{
    Queue_t *q = (Queue_t *)kmalloc(sizeof(Queue_t));
    if (!q)
        return NULL;
    
    q->front = q->rear = 0;
    q->count = 0;
    q->lock = 0;

    return q;
}

void queue_enqueue(Queue_t *q, void *data)
{
    lock_acquire(&q->lock);

    QueueNode_t *tmp = (QueueNode_t *)kmalloc(sizeof(QueueNode_t *));
    tmp->data = data;
    tmp->next = NULL;

    if (!q->rear)
        q->front = q->rear = tmp;
    else
    {
        q->rear->next = tmp;
        q->rear = tmp;
    }
    
    q->count++;
    lock_release(&q->lock);
}

void *queue_deqeueue(Queue_t *q)
{
    if (!q->count)
        return NULL;
    
    lock_acquire(&q->lock);

    QueueNode_t *tmp = q->front;
    q->front = q->front->next;
    if (!q->front)
        q->front = q->rear = NULL;
    
    void *ret = tmp->data;
    kfree(tmp);
    q->count--;

    lock_release(&q->lock);
    return ret;
}

void queue_remove(Queue_t *q, void *data)
{
    lock_acquire(&q->lock);

    QueueNode_t *tmp = q->front, *prev = NULL;
    while (tmp)
    {
        if (tmp->data == data)
        {
            if (!prev)
                q->front = tmp->next;
            else
                prev->next = tmp->next;
            
            q->count--;
            kfree(tmp);

            lock_release(&q->lock);
            return;
        }

        prev = tmp;
        tmp = tmp->next;
    }
}