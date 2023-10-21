Command Line Usage
==================

Synopsis
--------

``python -m yasimavr [arguments] firmware``

*yasimavr* will load a firmware binaries into a model and run until the simulation stops and then exits.

The minimum required arguments to provide are the firmware file [firmware], the MCU model [-m] and the frequency [-f].

GDB vs. Standalone Mode
-----------------------

In command line, *yasimavr* can be used in either standalone or GDB mode. The mode is set by the option [-g]

In standalone mode, *yasimavr* will :

* Instantiate a MCU model and load the firmware.
* Run the simulation cycles until a stop condition is met or the maximum nb of cycles is reached.
* If enabled, dump the final model state in a text file.
* Exit.

In GDB mode, *yasimavr* will :

* Instantiate a MCU model and load the firmware.
* Open a socket and listen for GDB connection.
* Once a GDB connection is open, run the simulation cycles as instructed by the GDB front end.
* When GDB closes the connection, if enabled, dump the final model state in a text file and exit.

Options
-------

.. program:: yasimavr

.. option:: firmware

Firmware file (in ELF binary format) to load into the MCU simulation model.

.. option:: -f,--frequency FREQ

Set the MCU clock frequency in Hertz. Required for yasimavr to run a simulation.

.. option:: -m,--mcu MODEL

Set the MCU model to be used for the simulation.
The model name is case insensitive.
Required for yasimavr to run a simulation.
Use the option --list-models to know the supported values for MODEL.

.. option:: --list-models

Print the list of supported MCU model names and exit immediately. No simulation is run.

.. option:: -c,--cycles N

Limit the simulation to run N cycles (roughly equivalent to MCU clock cycles).
If not set, the simulation will run indefinitely until a stop condition occurs:

* A device crash.
* A GDB connection closed.
* A inconditional sleep.

.. option:: -g,--gdb [PORT]

Enable the GDB mode.

*yasimavr* will listen for GDB connection on <localhost>:PORT. By default PORT is 1234.

.. option:: -v,--verbose LEVEL

Set the verbosity level. LEVEL values can be 0 (no verbosity) to 4 (maximum verbosity).

.. option:: -a,--analog [VCC]

Enable analog features with VCC as main supply voltage in Volts (default is 5.0 Volts)

.. option:: -r,--reference REF

Set the Analog voltage reference, relative to VCC (default is 1, i.e. AREF=VCC)

.. option:: -d,--dump PATH

On exit, dump the final state into a human-readable text file.

.. option:: -o,--output PATH

VCD file to record the traced variables

.. option:: -t,--trace KIND ARGS

Add a variable to be traced. KIND can be one of: ``pin``, ``port``, ``data``, ``vector`` or ``signal``

* pin X[/VARNAME] :
    Trace of the digital state (0/1/Z) of one GPIO pin.

    * X is the pin generic name (ex: ``PA0``, ``PD5``)
    * VARNAME is the variable name as it appears in the VCD file. If not specified, it is the pin generic name.

* port X[/VARNAME] :
    Trace of a combination of the digital states of all the pins comprising a GPIO port and combined as a 8-bits integer.

    For example with the port A, the variable will be a 8-bits integer made of [bit0=PA0, bit1=PA1, ..., bit7=PA7]. If some pins don't exist, the correspond bit will remain at 0.

    * X is the port letter (ex: ``A``, ``B``)

    * VARNAME is the variable name as it appears in the VCD file. If not specified, it is the port letter.

* data ADDR[/VARNAME] :
    Trace of a internal 8-bits data.

    * ADDR is the address in data space.
    * VARNAME is the variable name as it appears in the VCD file. If not specified, it is the memory address in hexadecimal format.

    Currently, it only works if the address corresponds to a SRAM location.

* vector N[/VARNAME] :
    Trace of an interrupt vector.
    The variable will be output as a boolean that is set when the vector flag is set by the corresponding peripheral, and cleared when it is acknowledged by the CPU (when about to jump to the corresponding ISR) or cancelled (usually by clearing the corresponding bit in registers).

    * N is the vector index
    * VARNAME is the variable name as it appears in the VCD file. If not specified, it is the vector index.

    | example with a ATMega328 model: ``vector 16/T0_OVF``
    | Traces the OVerFlow flag of Timer/Counter 0 on ATMega328.

* signal CTL/[size=SIZE],[id=ID],[ix=IX],[name=VARNAME]
    Generic peripheral signal tracing

    * CTL: Peripheral identifier from the model description file
    * SIZE: optional, variable size in bits (default is 32)
    * ID: optional, sigid to filter (default is no filter)
    * IX: optional, index to filter (default is no filter)

    | example with a ATMega328 model: ``signal TC_0/size=1,ID=2,IX=0,name=OCA``
    | Traces the OutputCompare channel A from Timer/Counter 0.

