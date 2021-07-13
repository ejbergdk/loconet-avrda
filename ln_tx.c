/*
 * ln_tx.c
 *
 * Created: 12-07-2021 16:45:46
 *  Author: Mikael Ejberg Pedersen
 */

#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "hal_ln.h"
#include "ln_def.h"
#include "ln_tx.h"


static int8_t opc_sw(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx, uint8_t op)
{
    lnpacket_t     *p = hal_ln_packet_get();

    if (!p)
        return -1;

    adr--;
    p->sw.op = op;
    p->sw.adrl = adr;
    p->sw.zero1 = 0;
    p->sw.adrh = adr >> 7;
    p->sw.on = on ? 1 : 0;
    p->sw.dir = dir ? 1 : 0;
    p->sw.zero2 = 0;
    hal_ln_send(p, cb, ctx);

    return 0;
}

int8_t ln_tx_opc_sw_req(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx)
{
    return opc_sw(adr, dir, on, cb, ctx, OPC_SW_REQ);
}

int8_t ln_tx_opc_sw_state(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx)
{
    return opc_sw(adr, dir, on, cb, ctx, OPC_SW_STATE);
}

int8_t ln_tx_opc_sw_ack(uint16_t adr, bool dir, bool on, hal_ln_tx_done_cb_t * cb, void *ctx)
{
    return opc_sw(adr, dir, on, cb, ctx, OPC_SW_ACK);
}

int8_t ln_tx_opc_input_rep(uint16_t adr, bool l, hal_ln_tx_done_cb_t * cb, void *ctx)
{
    lnpacket_t     *p = hal_ln_packet_get();

    if (!p)
        return -1;

    adr--;
    p->input_rep.op = OPC_INPUT_REP;
    p->input_rep.adrl = adr >> 1;
    p->input_rep.zero1 = 0;
    p->input_rep.adrh = adr >> 8;
    p->input_rep.l = l ? 1 : 0;
    p->input_rep.i = adr & 0x01;
    p->input_rep.x = 1;
    p->input_rep.zero2 = 0;
    hal_ln_send(p, cb, ctx);

    return 0;
}
