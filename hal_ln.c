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
#include "ac.h"
#include "ccl.h"
#include "fifo.h"
#include "hal_ln.h"
#include "ln_def.h"

#define BAUDRATE    16667UL
#define BAUD_REG    ((64 * F_CPU + 8 * BAUDRATE) / (16 * BAUDRATE))


/************************************************************************/
/* Statistics variables                                                 */
/************************************************************************/

#ifdef LNSTAT
typedef struct
{
    uint8_t         tx_max_attempts;
    uint16_t        tx_total;
    uint16_t        tx_success;
    uint16_t        tx_fail;
    uint16_t        tx_collisions;
    uint16_t        rx_success;
    uint16_t        rx_checksum;
    uint16_t        rx_partial;
    uint16_t        rx_extradata;
    uint16_t        rx_collisions;
    uint16_t        rx_nomem;
} stat_t;

static stat_t   stat;
#endif


/************************************************************************/
/* LocoNet packet handling                                              */
/************************************************************************/

/**
 * Number of LocoNet packets allocated.
 */
#define LNPACKET_CNT    8

/**
 * Packet structure for LocoNet and housekeeping data.
 */
typedef struct
{
    fifo_t          fifo;       // Fifo data
    hal_ln_tx_done_cb *cb;      // Callback function pointer
    void           *ctx;        // Callback context pointer
    hal_ln_result_t res;        // Result code
    lnpacket_t      lndata;     // LocoNet data
} packet_t;

#define PACKET_FROM_FIFO(x) ((packet_t *)((uint8_t *)(x) - offsetof(packet_t, fifo)))
#define PACKET_FROM_LN(x) ((packet_t *)((uint8_t *)(x) - offsetof(packet_t, lndata)))

static packet_t packets[LNPACKET_CNT];

static fifo_queue_t queue_free = { NULL, NULL };
static fifo_queue_t queue_rx = { NULL, NULL };
static fifo_queue_t queue_tx = { NULL, NULL };
static fifo_queue_t queue_done = { NULL, NULL };


/*
 * Inline version of hal_ln_packet_get for use in interrupts.
 */
__attribute__((always_inline))
static inline packet_t *packet_get_irq(void)
{
    fifo_t         *p;

    p = fifo_queue_get_irq(&queue_free);
    if (p)
        return PACKET_FROM_FIFO(p);
    return NULL;
}

lnpacket_t     *hal_ln_packet_get(void)
{
    fifo_t         *p;

    p = fifo_queue_get(&queue_free);
    if (p)
        return &PACKET_FROM_FIFO(p)->lndata;
    return NULL;
}

void hal_ln_packet_free(lnpacket_t *p)
{
    packet_t       *packet;

    packet = PACKET_FROM_LN(p);
    fifo_queue_put(&queue_free, &packet->fifo);
}

uint8_t hal_ln_packet_len(const lnpacket_t *packet)
{
    switch (packet->hdr.op & 0x60)
    {
    case 0x00:
    default:
        return 2;
    case 0x20:
        return 4;
    case 0x40:
        return 6;
    case 0x60:
        return packet->hdr.len;
    }
}


/************************************************************************/
/* LocoNet transmitting section                                         */
/************************************************************************/

/**
 * Attempts for transmitting a packet before giving up.
 * LN specification only states AT LEAST 25.
 */
#define TX_ATTEMPTS_MAX 50

static packet_t *tx_buf = NULL;
static uint8_t  tx_len;
static uint8_t  tx_idx;
static uint16_t tx_delay;
static uint8_t  tx_attempt;


/*
 * Start transmitting packet.
 */
__attribute__((always_inline))
static inline void tx_start(void)
{
    ccl_collision_clear();
    PORTA.OUTSET = PIN4_bm;     // XDIR = 1
    USART0.TXDATAL = tx_buf->lndata.raw[0];
    tx_idx = 1;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        USART0.CTRLA |= USART_DREIE_bm; // Enable data register empty interrupt
    }
    tx_attempt++;
}

/*
 * Set timer for next transmission attempt.
 */
__attribute__((always_inline))
static inline void tx_arm_timer(uint16_t cnt)
{
    if (TCB2.CNT >= (cnt - 1))
        cnt = TCB2.CNT + 2;
    TCB2.CCMP = cnt;
    TCB2.INTFLAGS = TCB_CAPT_bm;        // Clear capture interrupt flag
    TCB2.INTCTRL = TCB_CAPT_bm; // Enable capture interrupt
}

/*
 * Timer interrupt.
 * CD BACKOFF time has elapsed. Start transmitting.
 */
ISR(TCB2_INT_vect)
{
    TCB2.INTCTRL = 0;           // Disable capture interrupt
    tx_start();
}

/*
 * Data register empty interrupt.
 * Send next byte to USART.
 */
ISR(USART0_DRE_vect)
{
    if (!ccl_collision())
    {
        USART0.TXDATAL = tx_buf->lndata.raw[tx_idx++];
        if (tx_idx < tx_len)
            return;
    }

    // Last byte has been written to USART
    // Clear tx complete interrupt flag
    USART0.STATUS = USART_TXCIF_bm;

    // Disable data register empty and enable tx complete interrupt
    uint8_t         tmp = USART0.CTRLA;

    tmp &= ~USART_DREIE_bm;
    tmp |= USART_TXCIE_bm;
    USART0.CTRLA = tmp;
}

/*
 * TX complete interrupt.
 * End packet transmission and check for collision.
 */
ISR(USART0_TXC_vect)
{
    uint8_t         fail = 0;

    PORTA.OUTCLR = PIN4_bm;     // XDIR = 0
    USART0.CTRLA &= ~USART_TXCIE_bm;

    if (ccl_collision())
    {
#ifdef LNSTAT
        stat.tx_collisions++;
#endif

        if (tx_attempt < TX_ATTEMPTS_MAX)
        {
            if (tx_delay > CD_BACKOFF_MIN)
            {
                // Subtract 0.5 to 1 bit time from delay, and try again
                tx_delay -= (30 / CD_TICK_TIME) + (ccl_rnd() & 0x03);
                if (tx_delay < CD_BACKOFF_MIN)
                    tx_delay = CD_BACKOFF_MIN;
            }
            tx_arm_timer(tx_delay);
            return;
        }

        fail = 1;
    }

    // Determine if packet tx was successful or not
    if (fail == 0)
    {
        tx_buf->res = HAL_LN_SUCCESS;
#ifdef LNSTAT
        stat.tx_success++;
#endif
    }
    else
    {
        tx_buf->res = HAL_LN_FAIL;
#ifdef LNSTAT
        stat.tx_fail++;
#endif
    }

    // Put sent packet in done queue, for further processing outside interrupt
    fifo_queue_put_irq(&queue_done, &tx_buf->fifo);
    tx_buf = NULL;

#ifdef LNSTAT
    if (stat.tx_max_attempts < tx_attempt)
        stat.tx_max_attempts = tx_attempt;
#endif
}

void hal_ln_send(lnpacket_t *lnpacket, hal_ln_tx_done_cb *cb, void *ctx)
{
    uint8_t         len, cksum;
    uint8_t        *data;
    packet_t       *packet;

    len = hal_ln_packet_len(lnpacket) - 2;
    cksum = ~lnpacket->raw[0];
    data = &lnpacket->raw[1];
    while (len--)
    {
        *data &= 0x7f;
        cksum ^= *data++;
    }
    *data = cksum;

    packet = PACKET_FROM_LN(lnpacket);
    packet->cb = cb;
    packet->ctx = ctx;
    fifo_queue_put(&queue_tx, &packet->fifo);
}

/*
 * Handle tx queue.
 */
static void tx_update(void)
{
    fifo_t         *packetfifo;

    if (tx_buf)
        return;                 // Tx in progress

    // Tx idle: Send next packet in queue

    packetfifo = fifo_queue_get(&queue_tx);
    if (!packetfifo)
        return;                 // No packets in tx queue

#ifdef LNSTAT
    stat.tx_total++;
#endif

    tx_buf = PACKET_FROM_FIFO(packetfifo);
    tx_len = hal_ln_packet_len(&tx_buf->lndata);
    tx_delay = CD_BACKOFF_MAX;
    tx_attempt = 0;

    // Check if transmit is allowed now
    if ((TCB2.CNT >= tx_delay) && (TCB2.STATUS & TCB_RUN_bm))
        tx_start();
    else                        // if not, set timer to start tx when it is allowed
        tx_arm_timer(tx_delay);
}

/*
 * Handle tx done queue (callback and packet freeing).
 */
static void tx_done_update(void)
{
    fifo_t         *packetfifo;
    packet_t       *packet;

    packetfifo = fifo_queue_get(&queue_done);
    if (!packetfifo)
        return;                 // No packets in done queue

    packet = PACKET_FROM_FIFO(packetfifo);
    if (packet->cb)
        packet->cb(packet->ctx, packet->res);   // Tx done callback

    fifo_queue_put(&queue_free, packetfifo);
}


/************************************************************************/
/* LocoNet receiving section                                            */
/************************************************************************/

typedef enum
{
    RXS_IDLE,
    RXS_DATA
} rx_state_t;


/*
 * RX complete interrupt.
 */
__attribute__((flatten)) ISR(USART0_RXC_vect)
{
    static packet_t *buf = NULL;
    static rx_state_t state = RXS_IDLE;
    static uint8_t  idx = 0;
    static uint8_t  cksum;
    static uint8_t  len;
    uint8_t         data;
    uint8_t         status;

    status = USART0.RXDATAH;
    data = USART0.RXDATAL;

    if ((status & USART_FERR_bm))       // Framing error: Restart rx packet
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
            buf->lndata.raw[0] = data;
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
        buf->lndata.raw[idx++] = data;
        cksum ^= data;
        if (idx == 2)
            len = hal_ln_packet_len(&buf->lndata);
        if (idx >= len)
        {
            // Full packet received. Check checksum
            if (cksum == 0xff)
            {
                // Packet valid. Put in rx queue
                fifo_queue_put_irq(&queue_rx, &buf->fifo);
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

lnpacket_t     *hal_ln_receive(void)
{
    fifo_t         *p;

    p = fifo_queue_get(&queue_rx);
    if (p)
        return &PACKET_FROM_FIFO(p)->lndata;
    return NULL;
}


/************************************************************************/
/* Common stuff                                                         */
/************************************************************************/

void hal_ln_init(void)
{
    // Init analog comparator and configurable logic
    ac_init();
    ccl_init();

    // Init USART pins
    PORTA.DIRCLR = PIN1_bm;     // RX input
    PORTA.OUTCLR = PIN4_bm;
    PORTA.DIRSET = PIN4_bm;     // TX active (manual XDIR pin with less delay)

    // Init USART
    USART0.CTRLA = USART_RXCIE_bm | USART_RS485_DISABLE_gc;     // Enable rx complete interrupt
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
    USART0.BAUD = BAUD_REG;
    USART0.DBGCTRL = USART_DBGRUN_bm;
    USART0.EVCTRL = 0;
    USART0.STATUS = USART_RXCIF_bm | USART_TXCIF_bm;    // Clear interrupt flags
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_NORMAL_gc;

    // Init packet queue
    for (uint8_t i = 0; i < LNPACKET_CNT; i++)
        fifo_queue_put(&queue_free, &packets[i].fifo);
}

void hal_ln_update(void)
{
    tx_update();
    tx_done_update();
}


/************************************************************************/
/* Debug command related stuff.                                         */
/************************************************************************/

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
            lnpacket_t     *txdata;
            uint16_t        adr;
            uint8_t         occupied;

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

            hal_ln_send(txdata, NULL, NULL);
            break;
        }

    case 't':
        {
            lnpacket_t     *txdata;
            uint8_t         len;

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
            hal_ln_send(txdata, NULL, NULL);
            break;
        }

#ifdef LNSTAT
    case 's':
        {
            stat_t          s;

            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                s = stat;
            }
            printf_P(PSTR("TX:\n"));
            printf_P(PSTR(" Packets scheduled:  %u\n"), s.tx_total);
            printf_P(PSTR(" Packets sent:       %u\n"), s.tx_success);
            printf_P(PSTR(" Packets tx fail:    %u\n"), s.tx_fail);
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
            printf_P(PSTR(" Free packets:       %u\n"), fifo_queue_size(&queue_free));
            printf_P(PSTR(" In tx queue:        %u\n"), fifo_queue_size(&queue_tx));
            printf_P(PSTR(" In rx queue:        %u\n"), fifo_queue_size(&queue_rx));
            printf_P(PSTR(" In done queue:      %u\n"), fifo_queue_size(&queue_done));
            break;
        }
#endif

    default:
        printf_P(PSTR("Unknown argument\n"));
        break;
    }
}
