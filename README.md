# loconet-avrda
Experimental library providing LocoNet communication on AVR DA processors.

This library is used for ongoing tests and experiments of interfacing a Microchip AVR DA processor to a LocoNet bus, using as little hardware as possible.
See the [project homepage](https://www.ejberg.dk/portfolio/loconet-avr-da/) for more information on the hardware side.

hal_ln.* and ln_def.h are the main library files. ac.* ccl.* and fifo.* are required files, but should not be accessed from the outside world.

ln_rx.* are **optional** and intended to ease reception of LocoNet packets by decoding packet parameters and calling separate functions per packet opcode.
Warning: ln_rx.* are very much work in progress and may change drastically in its implementation.

Yes, I know. Documentation is scarce right now. Work is in progress.
