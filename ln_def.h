/*
 * ln_def.h
 *
 * Created: 08-02-2021 17:05:05
 *  Author: Mikael Ejberg Pedersen
 */ 


#ifndef LN_DEF_H_
#define LN_DEF_H_

#include <stdint.h>


/*
 * OpCode defines
 */

// 2-byte messages
#define OPC_BUSY            0x81
#define OPC_GPOFF           0x82
#define OPC_GPON            0x83
#define OPC_IDLE            0x85

// 4-byte messages
#define OPC_LOCO_SPD        0xa0
#define OPC_LOCO_DIRF       0xa1
#define OPC_LOCO_SND        0xa2
#define OPC_SW_REQ          0xb0
#define OPC_SW_REP          0xb1
#define OPC_INPUT_REP       0xb2
#define OPC_LONG_ACK        0xb4
#define OPC_SLOT_STAT1      0xb5
#define OPC_CONSIST_FUNC    0xb6
#define OPC_UNLINK_SLOTS    0xb8
#define OPC_LINK_SLOTS      0xb9
#define OPC_MOVE_SLOTS      0xba
#define OPC_RQ_SL_DATA      0xbb
#define OPC_SW_STATE        0xbc
#define OPC_SW_ACK          0xbd
#define OPC_LOCO_ADR        0xbf

// 6-byte messages (none)

// Variable length messages
#define OPC_PEER_XFER       0xe5
#define OPC_SL_RD_DATA      0xe7
#define OPC_IMM_PACKET      0xed
#define OPC_WR_SL_DATA      0xef

/*
 * Largest size for a LocoNet package, including op and cksum.
 */
#define LNPACKET_SIZE_MAX   127

/*
 * LocoNet packet header.
 */
typedef struct
{
    uint8_t         op;
    uint8_t         len;
} lnpacket_hdr_t;

/*
 * LocoNet packet address field.
 * Common format used in several packets.
 */
typedef struct
{
    uint8_t         op;
    uint8_t         adrl:7;
    uint8_t         zero1:1;
    uint8_t         adrh:4;
    uint8_t         zero2:4;
} lnpacket_adr_t;

/*
 * LocoNet packet for OPC_SW_REQ, OPC_SW_STATE and OPC_SW_ACK
 */
typedef struct
{
    uint8_t         op;
    uint8_t         adrl:7;
    uint8_t         zero1:1;
    uint8_t         adrh:4;
    uint8_t         on:1;
    uint8_t         dir:1;
    uint8_t         zero2:2;
} lnpacket_sw_t;

/*
 * LocoNet packet for OPC_SW_REP
 */
typedef struct
{
    uint8_t         op;
    uint8_t         adrl:7;
    uint8_t         zero1:1;
    uint8_t         adrh:4;
    uint8_t         lt:1;   // L if sel=1, T if sel=0
    uint8_t         ic:1;   // I if sel=1, C if sel=0
    uint8_t         sel:1;
    uint8_t         zero2:1;
} lnpacket_sw_rep_t;

/*
 * LocoNet packet for OPC_INPUT_REP
 */
typedef struct
{
    uint8_t         op;
    uint8_t         adrl:7;
    uint8_t         zero1:1;
    uint8_t         adrh:4;
    uint8_t         l:1;
    uint8_t         i:1;
    uint8_t         x:1;
    uint8_t         zero2:1;
} lnpacket_input_rep_t;

/*
 * LocoNet packet for OPC_LONG_ACK
 */
typedef struct
{
    uint8_t         op;
    uint8_t         lopc;
    uint8_t         ack1;
} lnpacket_long_ack_t;

/*
 * LocoNet packet for OPC_SLOT_STAT1
 */
typedef struct
{
    uint8_t         op;
    uint8_t         slot;
    uint8_t         stat1;
} lnpacket_slot_stat1_t;

/*
 * LocoNet packet for OPC_CONSIST_FUNC
 */
typedef struct
{
    uint8_t         op;
    uint8_t         slot;
    uint8_t         dirf;
} lnpacket_consist_func_t;

/*
 * LocoNet packet for OPC_UNLINK_SLOTS
 */
typedef struct
{
    uint8_t         op;
    uint8_t         sl1;
    uint8_t         sl2;
} lnpacket_unlink_slots_t;

/*
 * LocoNet packet for OPC_LINK_SLOTS
 */
typedef struct
{
    uint8_t         op;
    uint8_t         sl1;
    uint8_t         sl2;
} lnpacket_link_slots_t;

/*
 * LocoNet packet for OPC_MOVE_SLOTS
 */
typedef struct
{
    uint8_t         op;
    uint8_t         src;
    uint8_t         dst;
} lnpacket_move_slots_t;

/*
 * LocoNet packet for OPC_RQ_SL_DATA
 */
typedef struct
{
    uint8_t         op;
    uint8_t         slot;
    uint8_t         zero;
} lnpacket_rq_sl_data_t;

/*
 * LocoNet packet for OPC_LOCO_ADR
 */
typedef struct
{
    uint8_t         op;
    uint8_t         zero;
    uint8_t         adr;
} lnpacket_loco_adr_t;

/*
 * Unified LocoNet packet.
 * Contains all of the above.
 */
typedef union
{
    uint8_t                 raw[LNPACKET_SIZE_MAX];
    lnpacket_hdr_t          hdr;
    lnpacket_adr_t          adr;
    lnpacket_sw_t           sw;
    lnpacket_sw_rep_t       sw_rep;
    lnpacket_input_rep_t    input_rep;
    lnpacket_long_ack_t     long_ack;
    lnpacket_slot_stat1_t   slot_stat1;
    lnpacket_consist_func_t consist_func;
    lnpacket_unlink_slots_t unlink_slots;
    lnpacket_link_slots_t   link_slots;
    lnpacket_move_slots_t   move_slots;
    lnpacket_rq_sl_data_t   rq_sl_data;
    lnpacket_loco_adr_t     loco_adr;
} lnpacket_t;


#endif /* LN_DEF_H_ */