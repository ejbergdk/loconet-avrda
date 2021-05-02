/*
 * hal_ln.c
 *
 * Created: 31-05-2020 11:08:24
 *  Author: Mikael Ejberg Pedersen
 */ 

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/atomic.h>
#include "ccl.h"
#include "hal_ln.h"
#include "ln_def.h"

#define BAUDRATE    16667UL
#define BAUD_REG    ((64 * F_CPU + 8 * BAUDRATE) / (16 * BAUDRATE))


/*************/
/* Stat vars */

#ifdef LNSTAT
static uint8_t          tx_attempt;

typedef struct
{
    uint8_t             tx_max_attempts;
    uint16_t            tx_total;
    uint16_t            tx_success;
    uint16_t            tx_collisions;
    uint16_t            rx_success;
    uint16_t            rx_checksum;
    uint16_t            rx_partial;
    uint16_t            rx_extradata;
    uint16_t            rx_collisions;
    uint16_t            rx_nomem;
} stat_t;

static stat_t           stat;
#endif


/***************************/
/* LocoNet packets section */

#define LNPACKET_CNT    8

static lnpacket_t   packets[LNPACKET_CNT];
static lnpacket_t  *freepacket[LNPACKET_CNT];
static uint8_t      freepackets;

static inline __attribute__((always_inline)) lnpacket_t *packet_get_irq(void)
{
    lnpacket_t *p;

    if (freepackets > 0)
        p = freepacket[--freepackets];
    else
        p = NULL;
    return p;
}

static inline __attribute__((always_inline)) void packet_free_irq(lnpacket_t *p)
{
    if (freepackets < LNPACKET_CNT)
        freepacket[freepackets++] = p;
}

lnpacket_t *hal_ln_packet_get(void)
{
    lnpacket_t *p;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        p = packet_get_irq();
    }
    return p;
}

void hal_ln_packet_free(lnpacket_t *p)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        packet_free_irq(p);
    }
}

#define PACK_LEN(p)             \
    switch (p->hdr.op & 0x60)   \
    {                           \
        case 0x00:              \
        default:                \
            return 2;           \
        case 0x20:              \
            return 4;           \
        case 0x40:              \
            return 6;           \
        case 0x60:              \
            return p->hdr.len;  \
    }

uint8_t hal_ln_packet_len(const lnpacket_t *packet)
{
    PACK_LEN(packet)
}

/*
 * Inline version of hal_ln_packet_len for use in interrupts
 */
static inline __attribute__((always_inline)) uint8_t packet_len_irq(const lnpacket_t *packet)
{
    PACK_LEN(packet)
}


/**************/
/* TX section */

static lnpacket_t      *tx_buf = NULL;
static uint8_t          tx_len;
static uint8_t          tx_idx;
static uint16_t         tx_delay;

static lnpacket_t      *tx_queue[LNPACKET_CNT];
static uint8_t          txq_cnt = 0;

static inline __attribute__((always_inline)) void tx_start(void)
{
    ccl_collision_clear();
    PORTA.OUTSET = PIN4_bm;             // XDIR = 1
    USART0.TXDATAL = tx_buf->raw[0];
    tx_idx = 1;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        USART0.CTRLA |= USART_DREIE_bm; // Enable data register empty interrupt
    }
#ifdef LNSTAT
    tx_attempt++;
#endif
}

static inline __attribute__((always_inline)) void tx_arm_timer(uint16_t cnt)
{
    if (TCB2.CNT >= (cnt - 1))
        cnt = TCB2.CNT + 2;
    TCB2.CCMP = cnt;
    TCB2.INTFLAGS = TCB_CAPT_bm;    // Clear capture interrupt flag
    TCB2.INTCTRL = TCB_CAPT_bm;     // Enable capture interrupt
}

ISR(TCB2_INT_vect)
{
    TCB2.INTCTRL = 0;               // Disable capture interrupt
    tx_start();
}

ISR(USART0_DRE_vect)
{
    if (!ccl_collision())
    {
        USART0.TXDATAL = tx_buf->raw[tx_idx++];
        if (tx_idx < tx_len)
            return;
    }

    // Clear tx complete interrupt flag
    USART0.STATUS = USART_TXCIF_bm;

    // Disable data register empty and enable tx complete interrupt
    uint8_t tmp = USART0.CTRLA;
    tmp &= ~USART_DREIE_bm;
    tmp |= USART_TXCIE_bm;
    USART0.CTRLA = tmp;
}

ISR(USART0_TXC_vect)
{
    PORTA.OUTCLR = PIN4_bm;     // XDIR = 0
    USART0.CTRLA &= ~USART_TXCIE_bm;

    if (ccl_collision())
    {
        if (tx_delay > CD_BACKOFF_MIN)
        {
            tx_delay -= 2 + (ccl_rnd() & 0x03);
            if (tx_delay < CD_BACKOFF_MIN)
                tx_delay = CD_BACKOFF_MIN;
        }        
        tx_arm_timer(tx_delay);

#ifdef LNSTAT
        stat.tx_collisions++;
#endif
    }
    else
    {
        packet_free_irq(tx_buf);
        tx_buf = NULL;

#ifdef LNSTAT
        stat.tx_success++;
        if (stat.tx_max_attempts < tx_attempt)
            stat.tx_max_attempts = tx_attempt;
#endif
    }
}

void hal_ln_send(lnpacket_t *packet)
{
    static uint8_t  txq_widx = 0;
    uint8_t         len, cksum;
    uint8_t        *p;

    len = hal_ln_packet_len(packet) - 2;
    cksum = ~packet->raw[0];
    p = &packet->raw[1];
    while (len--)
    {
        *p &= 0x7f;
        cksum ^= *p++;
    }
    *p = cksum;

    tx_queue[txq_widx++] = packet;
    if (txq_widx >= LNPACKET_CNT)
        txq_widx = 0;
    txq_cnt++;
}

static void tx_update(void)
{
    static uint8_t  txq_ridx = 0;

    if (tx_buf != NULL || txq_cnt == 0)
        return;

    // Tx idle: Send next packet in queue
#ifdef LNSTAT
    stat.tx_total++;
    tx_attempt = 0;
#endif

    tx_buf = tx_queue[txq_ridx++];
    if (txq_ridx >= LNPACKET_CNT)
        txq_ridx = 0;
    txq_cnt--;
    tx_len = hal_ln_packet_len(tx_buf);
    tx_delay = CD_BACKOFF_MAX;

    // Check if transmit is allowed now
    if ((TCB2.CNT >= tx_delay) && (TCB2.STATUS & TCB_RUN_bm))
        tx_start();
    else    // if not, set timer to start tx when it is allowed
        tx_arm_timer(tx_delay);
}

/**************/
/* RX section */

typedef enum
{
    RXS_IDLE,
    RXS_DATA
} rx_state_t;

static lnpacket_t      *rx_queue[LNPACKET_CNT];
static volatile uint8_t rxq_cnt = 0;


ISR(USART0_RXC_vect)
{    
    static lnpacket_t  *buf = NULL;
    static rx_state_t   state = RXS_IDLE;
    static uint8_t      idx = 0;
    static uint8_t      cksum;
    static uint8_t      len;
    static uint8_t      rxq_widx = 0;
    uint8_t             data;
    uint8_t             status;

    status = USART0.RXDATAH;
    data = USART0.RXDATAL;

    if ((status & USART_FERR_bm))   // Framing error: Restart rx packet
    {
        state = RXS_IDLE;
        idx = 0;
#ifdef LNSTAT
        stat.rx_collisions++;
#endif
    }

    if (data & 0x80)
    {
        // Always restart reception when receiving opcode
        state = RXS_IDLE;
#ifdef LNSTAT
        if (idx != 0)
        {
            idx = 0;
            stat.rx_partial++;
        }        
#endif
    }    

    switch (state)
    {
        case RXS_IDLE:
        default:
            if (!buf)
            {
                buf = packet_get_irq();
                if (!buf)
                {
#ifdef LNSTAT
                    stat.rx_nomem++;
#endif
                    return;
                }
            }
            if (data & 0x80)
            {
                buf->raw[0] = data;
                cksum = data;
                idx = 1;
                state = RXS_DATA;
            }
            else
            {
#ifdef LNSTAT
                stat.rx_extradata++;
#endif
            }
            break;
            
        case RXS_DATA:
            buf->raw[idx++] = data;
            cksum ^= data;
            if (idx == 2)
                len = packet_len_irq(buf);
            if (idx >= len)
            {
                // Full packet received. Check checksum
                if (cksum == 0xff)
                {
                    // Packet valid. Put in rx queue
                    rx_queue[rxq_widx++] = buf;
                    if (rxq_widx >= LNPACKET_CNT)
                        rxq_widx = 0;
                    rxq_cnt++;
                    buf = NULL;
#ifdef LNSTAT
                    stat.rx_success++;
#endif
                }
                else
                {
#ifdef LNSTAT
                    stat.rx_checksum++;
#endif
                }
                state = RXS_IDLE;
                idx = 0;
            }                
            break;
    }
}

lnpacket_t *hal_ln_receive(void)
{
    static uint8_t  rxq_ridx = 0;
    lnpacket_t     *p;
    
    if (rxq_cnt == 0)
        return NULL;

    p = rx_queue[rxq_ridx++];
    if (rxq_ridx >= LNPACKET_CNT)
        rxq_ridx = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        rxq_cnt--;
    }
    return p;
}


/**********/
/* Common */

void hal_ln_init(void)
{
    // Init USART pins
    PORTA.DIRCLR = PIN1_bm;     // RX input
    PORTA.OUTCLR = PIN4_bm;
    PORTA.DIRSET = PIN4_bm;     // TX active (manual XDIR pin with less delay)
    // Init USART
    USART0.CTRLA = USART_RXCIE_bm | USART_RS485_DISABLE_gc; // Enable rx complete interrupt
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
    USART0.BAUD = BAUD_REG;
    USART0.DBGCTRL = USART_DBGRUN_bm;
    USART0.EVCTRL = 0;
    USART0.STATUS = USART_RXCIF_bm | USART_TXCIF_bm;    // Clear interrupt flags
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_NORMAL_gc;
    // Init LN packet list
    for (uint8_t i = 0; i < LNPACKET_CNT; i++)
        freepacket[i] = &packets[i];
    freepackets = LNPACKET_CNT;
}

void hal_ln_update(void)
{
    tx_update();
}


const __flash char cmdln_name[] = "ln";
const __flash char cmdln_help[] = "Loconet test";

void ln_cmd(uint8_t argc, char *argv[])
{
    if (argc < 2)
    {
        printf_P(PSTR("Missing argument\nArguments:\n"));
        printf_P(PSTR(" i <adr> <0/1> - Send input rep (feedback)\n"));
#ifdef LNSTAT
        printf_P(PSTR(" s - Stat\n"));
#endif
        printf_P(PSTR(" t <data1> [<datan>] - Tx packet\n"));
        return;
    }

    switch (argv[1][0])
    {
        case 'i':
        {
            lnpacket_t *txdata;
            uint16_t    adr;
            uint8_t     occupied;

            if (argc < 4)
            {
                printf_P(PSTR("Not enough arguments\n"));
                return;
            }

            txdata = hal_ln_packet_get();
            if (!txdata)
            {
                printf_P(PSTR("Out of lnpackets\n"));
                return;
            }

            adr = strtoul(argv[2], NULL, 0) - 1;
            occupied = strtoul(argv[3], NULL, 0);

            txdata->input_rep.op = OPC_INPUT_REP;
            txdata->input_rep.i = adr;
            txdata->input_rep.adrl = adr >> 1;
            txdata->input_rep.adrh = adr >> 8;
            txdata->input_rep.l = occupied ? 1 : 0;
            txdata->input_rep.x = 1;
            txdata->input_rep.zero1 = 0;
            txdata->input_rep.zero2 = 0;
            
            hal_ln_send(txdata);
            break;
        }

        case 't':
        {
            lnpacket_t *txdata;
            uint8_t     len;

            if (argc < 3)
            {
                printf_P(PSTR("Not enough arguments\n"));
                return;
            }

            txdata = hal_ln_packet_get();
            if (!txdata)
            {
                printf_P(PSTR("Out of lnpackets\n"));
                return;
            }

            len = argc - 2;
            for (uint8_t i = 0; i < len; i++)
                txdata->raw[i] = strtoul(argv[i + 2], NULL, 0);
            hal_ln_send(txdata);
            break;
        }

#ifdef LNSTAT
        case 's':
        {
            stat_t s;

            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                s = stat;
            }
            printf_P(PSTR("TX:\n"));
            printf_P(PSTR(" Packets scheduled:  %u\n"), s.tx_total);
            printf_P(PSTR(" Packets sent:       %u\n"), s.tx_success);
            printf_P(PSTR(" Collisions:         %u\n"), s.tx_collisions);
            printf_P(PSTR(" Max attemps for tx: %u\n"), s.tx_max_attempts);
            printf_P(PSTR("RX:\n"));
            printf_P(PSTR(" Packets received:   %u\n"), s.rx_success);
            printf_P(PSTR(" Checksum errors:    %u\n"), s.rx_checksum);
            printf_P(PSTR(" Partial packets:    %u\n"), s.rx_partial);
            printf_P(PSTR(" Extra bytes:        %u\n"), s.rx_extradata);
            printf_P(PSTR(" Collisions:         %u\n"), s.rx_collisions);
            printf_P(PSTR(" No memory:          %u\n"), s.rx_nomem);
            printf_P(PSTR("Mem:\n"));
            printf_P(PSTR(" Free packets:       %u\n"), freepackets);
            printf_P(PSTR(" In tx queue:        %u\n"), txq_cnt);
            printf_P(PSTR(" In rx queue:        %u\n"), rxq_cnt);
            break;
        }        
#endif

        default:
            printf_P(PSTR("Unknown argument\n"));
            break;
    }
}