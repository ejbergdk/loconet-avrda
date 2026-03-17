/*
 * configuration.h
 *
 * Configuration macros for settings that are used
 * throughout multiple source files.
 *
 * Created: 15-03-2026 14:36:53
 *  Author: Mikael Ejberg Pedersen
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

/**
 * Configuration for XDIR pin.
 *
 * The XDIR pin indicates if we are actively sending data on the LocoNet interface.
 * It is used by the CCL to activate the collision detection logic.
 * Note: The USARTs have a XDIR pin, but that is NOT used here. The XDIR pin from
 * the USARTs generate too much delay for us to be able to take the bus within 2 µs.
 * Instead, a GPIO pin is manually set when starting a transmission.
 *
 * The XDIR pin need to be configurable, as it may block other functionality,
 * like TWI (I2C) or SPI.
 *
 * The default configuration is PORTC pin 2.
 * Available pins on all AVR DA variants are:
 *   PORTA pin 2-6,
 *   PORTC pin 2,
 *   PORTD pin 0-1 and 4-7.
 * On 48 and 64 pin AVR's, PORTB pin 0-5 and PORTC pin 4-7 are also available.
 * PORTs E, F and G could in theory also be used, but will require changes
 * in the event routing channels.
 *
 * The following pins are NOT available for XDIR:
 * PORTA pin 0-1 are used by USART0, and pin 7 is used by AC1 out.
 * PORTB pin 6-7 can't connect the the event system, according to the AVR DA errata.
 * PORTC pin 0-1 are used by USART1, and pin 3 is used by CCL-LUT1 out.
 * PORTD pin 2 is used by AC1-AINP0, and pin 3 is used by CCL-LUT2/SEQ1 out (collision LED).
 *
 * Define XDIR_PORT to either PORTA, PORTB, PORTC or PORTD.
 * Define XDIR_PORT_ID to a single char identifing the port selected ('A', 'B', 'C' or 'D').
 * Define XDIR_PIN to a number between 0 and 7.
 */
#ifndef XDIR_PORT
#define XDIR_PORT       PORTC
#define XDIR_PORT_ID    'C'
#define XDIR_PIN        2
#endif

#endif /* CONFIGURATION_H_ */
