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
