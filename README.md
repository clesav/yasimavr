# yasimavr
Yet another simulator for Microchip AVR microcontrollers, inspired from simavr

------------

_yasimavr_ is a simulator for AVR 8-bits microcontrollers.
It is mainly aimed at the Mega0 and Mega1 series (ATmega80x-160x-320x-480x and others)
with a possibility to work with the "classic" series. (ATMega48/88/168/328 and others)

It is composed of 2 layers:
- a C++ layer for the core API and the various peripheral simulation models
- a Python layer to handle the configuration, utilities, data recording, and external
components

Compilation
------------
libelf is required to compile the shared libraries.

The python bindings for the C++ librairies are built with the SIP tool from RiverbankComputing
(https://www.riverbankcomputing.com)

Thanks
------------
Quite a few ideas in this software - and even big chunks of code - originate from simavr.
(https://github.com/buserror/simavr)
Big thanks to the simavr authors for this great tool !

Supported IOs
--------------
- GPIO
- SPI
- TWI
- USART

Supported Cores (very easy to add new ones!)
--------------------------------------------
+ ATMega328
+ ATMega 0-series (ATmega808/809/1608/1609/3208/3209/4808/4809)
+ more to come...

How to use
--------------
_yasimavr_ can be used as a Python package to run a prepared simulation script.
(See the examples for how it looks like)

It also supports direct command line run python -m yasimavr [options] [firmware]

For the list of command line options, execute python -m yasimavr -h

Some simple script examples are available here:
https://github.com/clesav/yasimavr/tree/main/examples
