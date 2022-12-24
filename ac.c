/*
 * ac.c
 *
 * Created: 17-05-2020 13:28:11
 *  Author: Mikael Ejberg Pedersen
 */

#include <avr/io.h>
#include "ac.h"

void ac_init(void)
{
    // Disable port pin digital input buffer
    PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
    // Setup Vref
    VREF.ACREF = VREF_REFSEL_1V024_gc;
    // Setup AC
    AC1.CTRLB = AC_WINSEL_DISABLED_gc;
    AC1.MUXCTRL = AC_MUXPOS_AINP0_gc | AC_MUXNEG_DACREF_gc;
    AC1.DACREF = 85;            // = 340mV @ 1.024V ref
    AC1.INTCTRL = 0;
    AC1.CTRLA = AC_RUNSTDBY_bm | AC_OUTEN_bm | AC_POWER_PROFILE0_gc | AC_HYSMODE_LARGE_gc | AC_ENABLE_bm;
}
