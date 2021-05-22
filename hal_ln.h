/*
 * hal_ln.h
 *
 * Created: 31-05-2020 11:08:35
 *  Author: Mikael Ejberg Pedersen
 */ 


#ifndef HAL_LN_H_
#define HAL_LN_H_

#include <stdint.h>
#include <stdio.h>
#include "ln_def.h"


/**
 * Callback result codes
 */
typedef enum
{
    HAL_LN_SUCCESS,
    HAL_LN_FAIL
} hal_ln_result_t;

/**
 * Callback type for tx done.
 */
typedef void (hal_ln_tx_done_cb)(void *, hal_ln_result_t res);

/**
 * Init LocoNet library.
 *
 * Call once from main program, before interrupts are enabled.
 */
extern void         hal_ln_init(void);

/**
 * Update LocoNet library.
 *
 * Call regularly from mainloop. No special timing constraints.
 */
extern void         hal_ln_update(void);

/**
 * Get pointer to an available LocoNet packet.
 *
 * The LocoNet packet is not empty, but may contain data from earlier use.
 * After use, the packet must be freed by either using hal_ln_packet_free,
 * or sending it on to another function thet eventually frees it.
 *
 * @return Pointer to LocoNet packet, or NULL if none is available.
 */
extern lnpacket_t  *hal_ln_packet_get(void);

/**
 * Return LocoNet packet to pool of free packets.
 *
 * @param p Pointer to LocoNet packet to free.
 */
extern void         hal_ln_packet_free(lnpacket_t *p);

/**
 * Length of LocoNet packet.
 *
 * Returns the length of a LocoNet packet (if packet contains valid data).
 *
 * @param p Pointer to LocoNet packet.
 * @return  LocoNet packet length, including checksum.
 */
extern uint8_t      hal_ln_packet_len(const lnpacket_t *packet);

/**
 * Send LocoNet packet.
 *
 * LocoNet packet is put in a queue, and sent as soon as possible.
 * LocoNet packet does not need the checksum, it is calculated by hal_ln_send.
 * All LocoNet timing, collision detection and retransmission requirements
 * are taken care of.
 * The function call returns immediately (before packet is sent).
 * An optional callback when packet is sent, is possible.
 *
 * @param lnpacket Pointer to LocoNet packet to send.
 *                 LocoNet packet is freed by function.
 * @param cb       Callback function for packet sent notification.
 *                 Set to NULL if not used.
 * @param ctx      Pointer to context data, that will be passed on to
 *                 the callback function.
 */
extern void         hal_ln_send(lnpacket_t *lnpacket, hal_ln_tx_done_cb *cb, void *ctx);

/**
 * Receive LocoNet packet.
 *
 * Received LocoNet packets are queued up, and can then be read one by one
 * by calling this function. Although there is a queue, the number of packets
 * that can be held is limited. Read received packets as soon as possible.
 *
 * @return Pointer to received LocoNet packet, or NULL if none is available.
 */
extern lnpacket_t  *hal_ln_receive(void);


/************************************************************************/
/* Debug command related stuff. Not to be used elsewhere.               */
/************************************************************************/
extern const __flash char cmdln_name[];
extern const __flash char cmdln_help[];
extern void         ln_cmd(uint8_t argc, char *argv[]);

#endif /* HAL_LN_H_ */
