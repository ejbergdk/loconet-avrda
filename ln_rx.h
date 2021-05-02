/*
 * ln_rx.h
 *
 * Created: 07-02-2021 15:25:34
 *  Author: Mikael Ejberg Pedersen
 */ 

#ifndef LN_RX_H_
#define LN_RX_H_

#include <stdint.h>

extern void         ln_rx_init(void);
extern void         ln_rx_update(void);

extern void         ln_rx_opc_sw_req(uint16_t adr, uint8_t dir, uint8_t on);
extern void         ln_rx_opc_sw_rep(uint16_t adr, uint8_t lt, uint8_t ic, uint8_t sel);
extern void         ln_rx_opc_input_rep(uint16_t adr, uint8_t l, uint8_t x);
extern void         ln_rx_opc_long_ack(uint8_t lopc, uint8_t ack1);
extern void         ln_rx_opc_sw_state(uint16_t adr, uint8_t dir, uint8_t on);
extern void         ln_rx_opc_sw_ack(uint16_t adr, uint8_t dir, uint8_t on);

extern void         ln_rx_opc_unknown(const lnpacket_t *p);

#endif /* LN_RX_H_ */
