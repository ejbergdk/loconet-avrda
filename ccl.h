/*
 * ccl.h
 *
 * Created: 21-05-2020 16:42:43
 *  Author: Mikael Ejberg Pedersen
 */ 


#ifndef CCL_H_
#define CCL_H_

#include <avr/io.h>
#include <stdint.h>

/**
 * Time between ticks on CD BACKOFF timer in us.
 */
#define CD_TICK_TIME    10

/**
 * Minimum ticks for CD backoff check.
 * Minimum time is 1560 us (26 bits) for slaves.
 */
#define CD_BACKOFF_MIN  (1560 / CD_TICK_TIME)

/**
 * Maximum ticks for CD backoff check.
 * Maximum time is 2760 us (46 bits) for slaves.
 */
#define CD_BACKOFF_MAX  (2760 / CD_TICK_TIME)

/**
 * Get collision detection flag.
 */
static inline __attribute__((always_inline)) uint8_t ccl_collision(void)
{
    return (TCB0.INTFLAGS & TCB_CAPT_bm);
}

/**
 * Clear collision detection flag.
 */
static inline __attribute__((always_inline)) void ccl_collision_clear(void)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
}

/**
 * Get pseudo-random number.
 */
static inline __attribute__((always_inline)) uint16_t ccl_rnd(void)
{
    return TCA0.SINGLE.CNT;
}

/**
 * Init CCL.
 *
 * Call once before interrupts are enabled.
 * NOTE: Handled from hal_ln.c
 */
extern void ccl_init(void);

#endif /* CCL_H_ */
