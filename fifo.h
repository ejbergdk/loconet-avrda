/*
 * fifo.h
 *
 * Created: 07-05-2021 17:40:37
 *  Author: Mikael Ejberg Pedersen
 */

#ifndef FIFO_H_
#define FIFO_H_

/**
 * Fifo data element.
 *
 * Must be a part of the data (usually a struct) to be put on a queue.
 */
typedef struct fifo_t_
{
    struct fifo_t_ *next;
} fifo_t;

/**
 * Queue handle.
 *
 * Use one per queue.
 * Initialize to NULL before using.
 */
typedef struct
{
    fifo_t         *head;
    fifo_t         *tail;
} fifo_queue_t;


/**
 * Inline version of fifo_queue_put for use in interrupts.
 *
 * @param queue Pointer to queue handle.
 * @param p     Pointer to fifo data element.
 */
__attribute__((always_inline))
static inline void fifo_queue_put_irq(fifo_queue_t *queue, fifo_t *p)
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

/**
 * Inline version of fifo_queue_get for use in interrupts.
 *
 * @param queue Pointer to queue handle.
 * @return      Pointer to fifo data element, or NULL if queue is empty.
 */
__attribute__((always_inline))
static inline fifo_t *fifo_queue_get_irq(fifo_queue_t *queue)
{
    fifo_t         *p;

    p = queue->head;
    if (p)
    {
        queue->head = p->next;
        if (!(queue->head))
            queue->tail = NULL;
    }

    return p;
}

/**
 * Put an item on a queue.
 *
 * @param queue Pointer to queue handle.
 * @param p     Pointer to fifo data element.
 */
extern void     fifo_queue_put(fifo_queue_t *queue, fifo_t *p);

/**
 * Get an item from a queue.
 *
 * @param queue Pointer to queue handle.
 * @return      Pointer to fifo data element, or NULL if queue is empty.
 */
extern fifo_t  *fifo_queue_get(fifo_queue_t *queue);

/**
 * Get number of elements in queue.
 *
 * @param queue Pointer to queue handle.
 * @return      Number of data elements in queue.
 */
extern uint8_t  fifo_queue_size(fifo_queue_t *queue);

#endif /* FIFO_H_ */
