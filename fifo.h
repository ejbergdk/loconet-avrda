/*
 * fifo.h
 *
 * Created: 07-05-2021 17:40:37
 *  Author: Mikael Ejberg Pedersen
 */ 

#ifndef FIFO_H_
#define FIFO_H_

typedef struct fifo_t_
{
    struct fifo_t_ *next;
} fifo_t;

typedef struct  
{
    fifo_t *head;
    fifo_t *tail;
} fifo_queue_t;


static inline __attribute__((always_inline))
void fifo_queue_put_irq(fifo_queue_t *queue, fifo_t *p)
{
    p->next = NULL;
    if (queue->tail)
    {
        queue->tail->next = p;
        queue->tail = p;
    }
    else
    {
        queue->head = queue->tail = p;
    }
}

static inline __attribute__((always_inline))
fifo_t * fifo_queue_get_irq(fifo_queue_t *queue)
{
    fifo_t *p;

    p = queue->head;
    if (p)
    {
        queue->head = p->next;
        if (!(queue->head))
            queue->tail = NULL;
    }

    return p;
}

extern void fifo_queue_put(fifo_queue_t *queue, fifo_t *p);
extern fifo_t * fifo_queue_get(fifo_queue_t *queue);
extern uint8_t fifo_queue_size(fifo_queue_t *queue);

#endif /* FIFO_H_ */
