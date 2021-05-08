/*
 * fifo.c
 *
 * Created: 07-05-2021 17:40:23
 *  Author: Mikael Ejberg Pedersen
 */ 

#include <stddef.h>
#include <stdint.h>
#include <util/atomic.h>
#include "fifo.h"

void fifo_queue_put(fifo_queue_t *queue, fifo_t *p)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        fifo_queue_put_irq(queue, p);
    }
}

fifo_t * fifo_queue_get(fifo_queue_t *queue)
{
    fifo_t *p;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        p = fifo_queue_get_irq(queue);
    }

    return p;
}

uint8_t fifo_queue_size(fifo_queue_t *queue)
{
    uint8_t cnt = 0;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        fifo_t *p = queue->head;

        while (p)
        {
            p = p->next;
            cnt++;
        }
    }

    return cnt;
}
