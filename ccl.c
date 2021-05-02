/*
 * ccl.c
 *
 * Created: 21-05-2020 16:42:34
 *  Author: Mikael Ejberg Pedersen
 */ 

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include "ccl.h"
#include "hal_ln.h"


void ccl_init(void)
{
    // Event channels setup
    // Ch0 UART 0 XDIR (PA4)
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN4_gc;
    EVSYS.USERCCLLUT0A = 1;     // Channel 0
    EVSYS.USERCCLLUT3A = 1;     // Channel 0

    // Ch1 AC1 out to TCB2 event in
    EVSYS.CHANNEL1 = EVSYS_CHANNEL1_AC1_OUT_gc;
    EVSYS.USERTCB2CAPT = 2;     // Channel 1

    // Ch2 LUT0 out to TCB0 event in
    EVSYS.CHANNEL2 = EVSYS_CHANNEL2_CCL_LUT0_gc;
    EVSYS.USERTCB0CAPT = 3;     // Channel 2
    
    // Ch3 TCB0 CAPT to LUT2 EventA
    EVSYS.CHANNEL3 = EVSYS_CHANNEL3_TCB0_CAPT_gc;
    EVSYS.USERCCLLUT2A = 4;     // Channel 3

    // Ch4 LUT2 out to TCB1 event in
    EVSYS.CHANNEL4 = EVSYS_CHANNEL4_CCL_LUT2_gc;
    EVSYS.USERTCB1CAPT = 5;     // Channel 4

    // Ch5 TCA0 overflow to TCB2 clock in
    EVSYS.CHANNEL5 = EVSYS_CHANNEL5_TCA0_OVF_LUNF_gc;
    EVSYS.USERTCB2COUNT = 6;     // Channel 5

    // Timer setup
    // TCB0 timeout check (delay) of LUT0 output
    TCB0.CTRLB = TCB_CNTMODE_TIMEOUT_gc;
    TCB0.CCMP = F_CPU * 15 / 1000000UL;         // 15 탎 (1/4 bit)
    TCB0.EVCTRL = TCB_CAPTEI_bm;
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;

    // TCB1 single-shot ASYNC
    TCB1.CTRLB = TCB_ASYNC_bm | TCB_CNTMODE_SINGLE_gc;
    TCB1.CCMP = (F_CPU * 60 / 1000000UL) * 15;  // 60 탎 * 15 bits
    TCB1.EVCTRL = TCB_CAPTEI_bm;
    TCB1.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;

    // TCA0 clock divider for CD backoff check in TCB2
    // Generate 100 kHz clock (10 탎)
    TCA0.SINGLE.CTRLD = 0;  // Disable split mode
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.CTRLC = 0;
    TCA0.SINGLE.CTRLECLR = TCA_SINGLE_DIR_bm;   // Count up
    TCA0.SINGLE.EVCTRL = 0; // No event input
    TCA0.SINGLE.INTCTRL = 0;    // No interrupts
    TCA0.SINGLE.PER = F_CPU * CD_TICK_TIME / 1000000UL - 1;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;

    // TCB2 CD backoff check
    TCB2.CTRLB = TCB_CNTMODE_TIMEOUT_gc;
    TCB2.EVCTRL = TCB_FILTER_bm | TCB_CAPTEI_bm;
    TCB2.INTCTRL = 0;   // No interrupts
    TCB2.CCMP = CD_BACKOFF_MAX; // Max CD backoff: 2760 탎 (46 bits)
    TCB2.CTRLA = TCB_CLKSEL_EVENT_gc | TCB_ENABLE_bm;

    // CCL setup
    CCL.CTRLA = CCL_ENABLE_bm;

    // LUT0 collision detector logic
    CCL.LUT0CTRLB = CCL_INSEL1_AC1_gc | CCL_INSEL0_USART0_gc;
    CCL.LUT0CTRLC = CCL_INSEL2_EVENTA_gc;
    CCL.TRUTH0 = 0x20;

    // LUT1 Tx output mux
    CCL.LUT1CTRLB = CCL_INSEL1_TCB1_gc | CCL_INSEL0_USART0_gc;
    CCL.LUT1CTRLC = CCL_INSEL2_LINK_gc;
    CCL.TRUTH1 = 0xC5;

    // Sequencer 0 disabled
    CCL.SEQCTRL0 = CCL_SEQSEL_DISABLE_gc;

    // LUT0/1 enable and lock
    CCL.LUT1CTRLA = CCL_OUTEN_bm | CCL_FILTSEL_DISABLE_gc | CCL_CLKSRC_CLKPER_gc | CCL_ENABLE_bm;
    CCL.LUT0CTRLA = CCL_FILTSEL_FILTER_gc | CCL_CLKSRC_CLKPER_gc | CCL_ENABLE_bm;

#ifdef CCLDEBUG
    // LUT2/3 sequencer output PD3  !!! DEBUG ONLY !!!
    PORTD.DIRSET = PIN3_bm;
#endif

    // LUT2 TCB0 CAPT event to sequencer S
    CCL.LUT2CTRLB = CCL_INSEL1_EVENTA_gc | CCL_INSEL0_FEEDBACK_gc;
    CCL.LUT2CTRLC = CCL_INSEL2_MASK_gc;
    CCL.TRUTH2 = 0x44;

    // LUT3 XDIR and BREAK done to sequencer R
    CCL.LUT3CTRLB = CCL_INSEL1_TCB1_gc | CCL_INSEL0_FEEDBACK_gc;
    CCL.LUT3CTRLC = CCL_INSEL2_EVENTA_gc;
    CCL.TRUTH3 = 0x02;

    // Sequencer 1 RS latch
    CCL.SEQCTRL1 = CCL_SEQSEL_RS_gc;

    // LUT2/3 and sequencer 1 enable and lock
    CCL.LUT3CTRLA = CCL_FILTSEL_FILTER_gc | CCL_CLKSRC_CLKPER_gc | CCL_ENABLE_bm;
#ifdef CCLDEBUG
    CCL.LUT2CTRLA = CCL_OUTEN_bm | CCL_FILTSEL_DISABLE_gc | CCL_CLKSRC_CLKPER_gc | CCL_ENABLE_bm;
#else
    CCL.LUT2CTRLA = CCL_FILTSEL_DISABLE_gc | CCL_CLKSRC_CLKPER_gc | CCL_ENABLE_bm;
#endif
}
