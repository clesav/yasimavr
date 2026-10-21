============
Introduction
============


.. include:: ../../README.rst


License
-------

`yasimavr` is licensed under the GPL-3.0 license.

`yasimavr` includes code copied and modified from `simavr` (https://github.com/buserror/simavr),
licensed under the GPL-3.0 license.

`yasimavr` includes code copied and modified from Microchip Device Family Packs,
licensed under the Apache License, Version 2.0.


Limitations
-----------

**Or what not to expect**:


**Security** :

  `yasimavr` should not be used to execute a firmware of unknown/untrusted origin. Although checks are in place to
  detect bad addressing and opcodes, they are designed for debugging, not security.

  The GDB server utility should only be used over a trusted network.


**Performance** :

  The design intent is to trade execution speed for simplicity, flexibility and hackability.
  The simulation engine has proven fast enough on modern computers for use cases seen so far and compiling options
  allow some degree of optimisation to improve the performance if necessary.
  The Python layer can be bypassed entirely in order to build a pure C/C++ software. However, much of the features
  to build and configure a device model would have to be manually written.


**Timing accuracy** :

  `yasimavr`'s design intent is to be in the right ballpark in term of Instructions Per Second (IPS) metrics
  but the execution times of the instruction set remain approximate and should not be relied on.
  As a result, the execution speed of firmware code may differ from real silicon.
  In particular, code such as AVR-LIBC's function ``_delay_us()`` that is designed
  with precise cycle counting of individual instructions may give inaccurate results.

  Similarly, `yasimavr` does not simulate the delays due to signal propagation or synchronisation within the microcontroller.


**Silicon accuracy** :

  `yasimavr`'s device and peripheral models are written and checked mostly against datasheets.
  Any erratum or undocumented behaviour is usually left out.

  Also, `yasimavr` is firmware-centric, meaning that device features that have no interaction with the
  CPU are usually left out.


**Electrical accuracy** :

  A crude model of the electrical behaviour of I/O pins is included but is limited to the strict minimum
  required to control GPIOs and connect to external basic components such as push buttons and leds.
  Any use beyond that is out of scope. `yasimavr` is not an electrical circuit simulator nor does it aim to be.
  Much better tools are available out there for that purpose.

