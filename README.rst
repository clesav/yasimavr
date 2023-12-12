yasimavr
========

Yet another simulator for Microchip AVR microcontrollers, inspired from simavr
------------------------------------------------------------------------------

_yasimavr_ is a simulator for AVR 8-bits microcontrollers.
It is mainly aimed at the Mega0 and Mega1 series (ATmega80x-160x-320x-480x and others)
with a possibility to work with the "classic" series. (ATMega48/88/168/328 and others)

It is composed of 2 layers:

* a C++ layer for the core API and the various peripheral simulation models
* a Python layer to handle the configuration, utilities, data recording, and external components

Installation
------------

Prerequisites:
**************

* Python (version >=3.7) and PIP
* For Ubuntu, libelf is required: ``sudo apt-get install libelf-dev``

Install:
********

* execute: ``pip install yasimavr``

The python bindings for the C++ librairies are built with the SIP tool from RiverbankComputing
(https://www.riverbankcomputing.com)

Thanks
------

Quite a few ideas in this software - and even big chunks of code - originate from simavr.
(https://github.com/buserror/simavr)
Big thanks to the simavr authors for this great tool !

Supported IOs
--------------

* GPIO
* SPI
* TWI
* USART

Supported Cores
---------------

The package includes a predefined set of MCU models:

* ATMegaxx8 series (ATMega48/88/168/328)
* ATMega 0-series (ATMega808/809/1608/1609/3208/3209/4808/4809)

Other device models can be easily simulated by creating a YAML config file.
A template is provided, and the example _atgiga4809_ shows how to load and use a customised device configuration.
New simulation models for peripherals can be created in Python or C++ using the provided API.

Features
--------

* Real-time/Fast mode : yasimavr can try to sync the simulated time with system time or run as fast as possible
* AVR-GDB integration : yasimavr can acts as a GDB backend stub, with support for breakpoints and watchpoints
* VCD export : yasimavr can export traces of pin states, GPIO ports, interrupt vectors, memory locations or generic signals in Value Change Dump (VCD) files
* MCU dump : at any point of the simulation, yasimavr can create a snapshot of the state of the MCU model, including all registers and memories and save it in a text file.
* "Zombie" mode : yasimavr can directly interact with simulated peripherals by acting as the CPU. This is useful to verify customised peripheral models or a test script.
* Probing : yasimavr can read/write CPU registers or memories on-the-fly. This is useful to force the firmware into certain branches for example, improving test coverage.

How to use
----------

_yasimavr_ can be used as a Python package to run a prepared simulation script.
(See the examples for how it looks like)

It also supports direct command line use:

* python -m yasimavr [options] [firmware]

For the list of command line options, execute python -m yasimavr -h

Some simple script examples are available here:
https://github.com/clesav/yasimavr/tree/main/examples

Documentation
-------------

The documentation is still a work in progress but will be progressively completed.
The online version, including an API reference, can be read on the Read the Docs:

* [Development documentation] http://yasimavr.readthedocs.io/en/latest/
* [Stable documentation] http://yasimavr.readthedocs.io/en/stable/
