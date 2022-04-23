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

------------
Compilation Requirements:
- MinGW64 (latest)
- libelf
- Python >= 3.7

The python bindings for the C++ librairies are built with the SIP tool from RiverbankComputing
(https://www.riverbankcomputing.com)

------------
Quite a few ideas in this software - and even big chunks of code - originate from simavr.
(https://github.com/buserror/simavr)
Big thanks to the simavr authors for this great tool !

Some examples are provided 

Supported IOs
--------------
- GPIO
- SPI
- TWI
- USART

Supported Cores (very easy to add new ones!)
--------------
+ ATMega328
+ ATMega4809
+ more to come...
