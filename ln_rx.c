/*
* ln_rx.c
*
* Created: 07-02-2021 15:25:17
*  Author: Mikael Ejberg Pedersen
*/

#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>
#include "hal_ln.h"
#include "ln_def.h"
#include "ln_rx.h"


#ifdef LNMONITOR
static const __flash char *opc_name(uint8_t opc)
{
    switch (opc)
    {
        case OPC_BUSY:
            return PSTR("OPC_BUSY");
        case OPC_GPOFF:
            return PSTR("OPC_GPOFF");
        case OPC_GPON:
            return PSTR("OPC_GPON");
        case OPC_IDLE:
            return PSTR("OPC_IDLE");
        case OPC_LOCO_SPD:
            return PSTR("OPC_LOCO_SPD");
        case OPC_LOCO_DIRF:
            return PSTR("OPC_LOCO_DIRF");
        case OPC_LOCO_SND:
            return PSTR("OPC_LOCO_SND");
        case OPC_SW_REQ:
            return PSTR("OPC_SW_REQ");
        case OPC_SW_REP:
            return PSTR("OPC_SW_REP");
        case OPC_INPUT_REP:
            return PSTR("OPC_INPUT_REP");
        case OPC_LONG_ACK:
            return PSTR("OPC_LONG_ACK");
        case OPC_SLOT_STAT1:
            return PSTR("OPC_SLOT_STAT1");
        case OPC_CONSIST_FUNC:
            return PSTR("OPC_CONSIST_FUNC");
        case OPC_UNLINK_SLOTS:
            return PSTR("OPC_UNLINK_SLOTS");
        case OPC_LINK_SLOTS:
            return PSTR("OPC_LINK_SLOTS");
        case OPC_MOVE_SLOTS:
            return PSTR("OPC_MOVE_SLOTS");
        case OPC_RQ_SL_DATA:
            return PSTR("OPC_RQ_SL_DATA");
        case OPC_SW_STATE:
            return PSTR("OPC_SW_STATE");
        case OPC_SW_ACK:
            return PSTR("OPC_SW_ACK");
        case OPC_LOCO_ADR:
            return PSTR("OPC_LOCO_ADR");
        case OPC_PEER_XFER:
            return PSTR("OPC_PEER_XFER");
        case OPC_SL_RD_DATA:
            return PSTR("OPC_SL_RD_DATA");
        case OPC_IMM_PACKET:
            return PSTR("OPC_IMM_PACKET");
        case OPC_WR_SL_DATA:
            return PSTR("OPC_WR_SL_DATA");
        default:
            break;
    }
    return PSTR("UNKNOWN");
}
#endif


void ln_rx_init(void)
{
}

void ln_rx_update(void)
{
    lnpacket_t     *p = hal_ln_receive();
    uint16_t        adr;

    if (!p)
        return;

#ifdef LNMONITOR
    const uint8_t  *data = p->raw;
    uint8_t         len = hal_ln_packet_len(p);
    printf_P(opc_name(p->hdr.op));
    while (len--)
        printf_P(PSTR(" %02x"), *data++);
    printf_P(PSTR("\n"));
#endif

    adr = (p->adr.adrl | (p->adr.adrh << 7)) + 1;
    
    switch (p->hdr.op)
    {
        case OPC_SW_REQ:
            ln_rx_opc_sw_req(adr, p->sw.dir, p->sw.on);
            break;

        case OPC_SW_REP:
            ln_rx_opc_sw_rep(adr, p->sw_rep.lt, p->sw_rep.ic, p->sw_rep.sel);
            break;

        case OPC_INPUT_REP:
            adr--;
            adr <<= 1;
            adr += p->input_rep.i;
            adr++;
            ln_rx_opc_input_rep(adr, p->input_rep.l, p->input_rep.x);
            break;

        case OPC_LONG_ACK:
            ln_rx_opc_long_ack(p->long_ack.lopc, p->long_ack.ack1);
            break;

        case OPC_SW_STATE:
            ln_rx_opc_sw_state(adr, p->sw.dir, p->sw.on);
            break;

        case OPC_SW_ACK:
            ln_rx_opc_sw_ack(adr, p->sw.dir, p->sw.on);
            break;

        // Still missing more OPC's here

        default:
            ln_rx_opc_unknown(p);
            break;
    }

    hal_ln_packet_free(p);
}


__attribute__((weak)) void ln_rx_opc_sw_req(uint16_t adr, uint8_t dir, uint8_t on)
{
#ifdef LNMONITOR
    printf_P(PSTR("  adr %u %c %S\n"), adr, dir ? 'G' : 'R', on ? PSTR("ON") : PSTR("OFF"));
#endif
}

__attribute__((weak)) void ln_rx_opc_sw_rep(uint16_t adr, uint8_t lt, uint8_t ic, uint8_t sel)
{
#ifdef LNMONITOR
    if (sel)
        printf_P(PSTR("  adr %u I=%u L=%u\n"), adr, ic, lt);
    else
        printf_P(PSTR("  adr %u C=%u T=%u\n"), adr, ic, lt);
#endif
}

__attribute__((weak)) void ln_rx_opc_input_rep(uint16_t adr, uint8_t l, uint8_t x)
{
#ifdef LNMONITOR
    if (x)
        printf_P(PSTR("  adr %u %S\n"), adr, l ? PSTR("occupied") : PSTR("free"));
    else
        printf_P(PSTR("  adr %u RESERVED FOR FUTURE USE\n"), adr);
#endif
}

__attribute__((weak)) void ln_rx_opc_long_ack(uint8_t lopc, uint8_t ack1)
{
#ifdef LNMONITOR
    printf_P(PSTR("  LOPC=0x%02x ACK1=0x%02x\n"), lopc, ack1);
#endif
}

__attribute__((weak)) void ln_rx_opc_sw_state(uint16_t adr, uint8_t dir, uint8_t on)
{
#ifdef LNMONITOR
    printf_P(PSTR("  adr %u %c %S\n"), adr, dir ? 'G' : 'R', on ? PSTR("ON") : PSTR("OFF"));
#endif
}

__attribute__((weak)) void ln_rx_opc_sw_ack(uint16_t adr, uint8_t dir, uint8_t on)
{
#ifdef LNMONITOR
    printf_P(PSTR("  adr %u %c %S\n"), adr, dir ? 'G' : 'R', on ? PSTR("ON") : PSTR("OFF"));
#endif
}



__attribute__((weak)) void ln_rx_opc_unknown(const lnpacket_t *p)
{
}
