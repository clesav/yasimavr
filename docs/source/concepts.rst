Key Concepts
==================

This page is mainly to try and explain some of the concepts underlying *yasimavr* internals and behaviour.


Signals/Hooks
-------------

Principles
**********

*yasimavr* implements a message passing mechanism calls Signal/Hook that is used extensively throughout the library.
A signal is connected to one or several hooks and provides callback functions that can carry optional metadata.
It is in principle very similar to *simavr*'s IRQ or Qt's Signal/Slot systems.

Each signal can be connected to one or more hook and one hook can be connected to one or more signals.

Hook callbacks are executed whenever a signal to which the hook is connected, is raised by using its ``raise()`` function.

Data details
************

The signature of hook callbacks is :
``void raised (const signal_data_t &sigdata, int hooktag)``

where sigdata has the following arguments:

* sigid : a integer defining the kind of signal raised. It is usually defined by a *SignalId* enumeration at the emitting class.

* index : a generic integer index

* data : generic data carried by the signal

The *hooktag* argument is an optional integer that is provided at connection and has only meaning for the hook callbacks. It is passed transparently by the signal mechanism.
It is usually used when a single hook is connected to several signal, in order to differentiate which signal is raised.

Data signals
************

The class DataSignal defined in *yasimavr.lib.core* add an extra feature on top of the signalling mechanism: it stores the data passed through.

It allows a signal consumer both to interrogate what is the current state of a specific signal ID and also to get notified when that state changes.


Peripheral Control Requests (CTLREQ)
------------------------------------

The CTLREQ mechanism is a query/response mechanism that allows a device model and its attached peripherals to communicate with each other with an emphasis on modularity and flexibility.

It is used for example to implement the sleep mechanisms:

* When executing a SLEEP instruction, the instruction interpreter sends a CTLREQ to the sleep controller.
* The sleep controller reads the registers to determine if sleep is allowed and in which mode.
* It then sends a CTLREQ to the device to enter the configured sleep mode and notify all peripherals.
* If no sleep controller is attached then the SLEEP instruction has simply no effect.

The signature of the CTLREQ callback is ``bool ctlreq (ctlreq_id_t req, ctlreq_data_t *data)``

The *req* argument is usually a CTLREQ_xxxx constant that identifies the query. The optional *data* argument
is used to carry both query data and response data.

The returned boolean indicates whether the request could be processed.


Pseudo-Sleep Mode
-----------------

The pseudo-sleep mode is a special sleep mode with no real-world meaning and that is triggered by executing the
instruction "RJMP .-2" - also known as infinite inconditional loop.

When a MCU executes this instruction, the Program Counter will freeze and all CPU registers will remain unchanged.
The only ways to get out of it are by an interrupt or a device reset.
From a functional point of view, this is equivalent to entering a sleep mode. Therefore *yasimavr* processes this
instruction as entering a special sleep mode called pseudo-sleep.

This concept allows *yasimavr* to simulate much faster firmwares that have interrupt-driven designs by skipping useless cycles. See the example **mega328_blink**, which uses a similar pattern.

This special mode is enabled by default and can be disabled by the option *DisablePseudoSleep* in the *Device* class.

.. note:: If the Global Interrupt Enable flag is cleared, the simulation will exit gracefully on entering pseudo-sleep mode (as with any actual sleep mode). This case is useful to stop the simulation when the firmware returns from the main() function. Often, the instructions "CLI; RJMP -.2" are added as a post-main stub by the linker. The simulation would detect that and exit.


Analog Features
---------------

The way *yasimavr* simulates a number of analog features of a MCU, such as ADC, analog comparators, voltage references, relies on a few conventions.

To enable analog features, VCC (the main power supply voltage for the MCU) must be
specified.
In command line, it is provided by the option ``-a``. For example the following will enable analog features and set VCC = 5.0 Volts:

.. code-block::

   > python -m yasimavr -a 5.0 firmware.elf

In a script, the field vcc of the Firmware object must be set to a non-zero value *before* loading it into the device model:

.. code-block::

   > fw = Firmware.read_elf("firmware.elf")
   > fw.vcc = 5.0
   > device.load_firmware(fw)

All other voltage values used within ``yasimavr`` are relative to VCC.
So, with VCC set to 5.0 Volts, a analog value of 1.0 must be read as 5.0 Volts,
or a value of 0.5, read as 2.5 Volts.

The values are restricted to the range [0.0; 1.0] which means that all simulated voltages are constrained to [0.0; VCC].
Any value specified outside this range will at best be clipped, at worst crash the software alltogether...

.. note::

   The only exception to the rule above is when configuring internal voltage references.
   On initialisation and configuration, these shall be passed on to the VREF model as absolute voltage values.
   However, during a simulation, whenever a reference is required, it is returned as relative to VCC.
