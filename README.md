# loconet-avrda
Experimental library providing LocoNet communication on AVR DA processors.

This library is used for ongoing tests and experiments of interfacing a Microchip AVR DA processor to a LocoNet bus, using as little hardware as possible.
See the [project homepage](https://www.ejberg.dk/portfolio/loconet-avr-da/) for more information on the hardware side.

hal_ln.\* and ln_def.h are the main library files. ac.\* ccl.\* and fifo.\* are required files, but should not be accessed from the outside world.

ln_rx.* are **optional** and intended to ease reception of LocoNet packets by decoding packet parameters and calling separate functions per packet opcode.
Warning: ln_rx.* are very much work in progress and may change drastically in its implementation.

### Preprocessor defines
Certain features of the library can be controlled by defining preprocessor macros.
These are usually passed to the gcc compiler with the `-D` command line option.

Name | Purpose
---- | -------
CCLDEBUG | Outputs sequencer 1 on pin PD3 (collision detected). Useful for logic analyzer captures
LNSTAT | Collect statistical data on LocoNet comms. Read stat with shell cmd `ln s`
LNMONITOR | Write all received LocoNet data on debug shell
LNECHO | Receive and process the echo of data sent from the library itself

And of course F_CPU should always be defined to the AVR's clock speed (in Hz).
This code has only been tested with the AVR running at 24 MHz.
Slower clock speeds should be possible, but it's not guaranteed.

Yes, I know. Documentation is scarce right now. Work is in progress.
