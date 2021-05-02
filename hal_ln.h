/*
 * lnuart.h
 *
 * Created: 31-05-2020 11:08:35
 *  Author: Mikael Ejberg Pedersen
 */ 


#ifndef HAL_LN_H_
#define HAL_LN_H_

#include <stdint.h>
#include <stdio.h>
#include "ln_def.h"


extern lnpacket_t  *hal_ln_packet_get(void);
extern void         hal_ln_packet_free(lnpacket_t *p);
extern uint8_t      hal_ln_packet_len(const lnpacket_t *packet);

extern void         hal_ln_init(void);
extern void         hal_ln_update(void);
extern void         hal_ln_send(lnpacket_t *packet);
extern lnpacket_t  *hal_ln_receive(void);

extern const __flash char cmdln_name[];
extern const __flash char cmdln_help[];
extern void         ln_cmd(uint8_t argc, char *argv[]);

#endif /* HAL_LN_H_ */
