/*
 * ln_tx.h
 *
 * Created: 12-07-2021 16:44:19
 *  Author: Mikael Ejberg Pedersen
 */

#ifndef LN_TX_H_
#define LN_TX_H_

#include <stdbool.h>
#include <stdint.h>
#include "hal_ln.h"

extern int8_t   ln_tx_opc_sw_req(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx);
extern int8_t   ln_tx_opc_sw_state(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx);
extern int8_t   ln_tx_opc_sw_ack(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx);
extern int8_t   ln_tx_opc_input_rep(uint16_t adr, bool l, hal_ln_tx_done_cb_t * cb, void *ctx);

/*
extern void     ln_tx_opc_sw_rep(uint16_t adr, uint8_t lt, uint8_t ic, uint8_t sel);
extern void     ln_tx_opc_long_ack(uint8_t lopc, uint8_t ack1);
*/

#endif /* LN_TX_H_ */
