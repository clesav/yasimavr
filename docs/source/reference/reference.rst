===================
Developer Reference
===================


This reference is useful to the developer for writing simulation scripts,
new core architectures, or new device/peripheral models.

`yasimavr` contains the following packages available to the user and simulation scripts :

* ``yasimavr.lib.core`` : Base types and common classes for a simulation model and environment.
* ``yasimavr.lib.arch_avr`` : Simulation models for "classic" AVR devices and peripherals (i.e. using the AVRe, AVRrc architectures)
* ``yasimavr.lib.arch_xt`` : Simulation models for AVR devices and peripherals using the AVRxt architecture
* ``yasimavr.device_library`` : Facilities for creating device models and definition of the builtin models
* ``yasimavr.utils`` : Python utilities for interacting with device models within simulation scripts


.. toctree::
   :maxdepth: 2
   :caption: Contents:

   corelib_basics
   corelib_advanced
   ctlreqs
   inputs_outputs
   lib_xt
   lib_avr
   device_library
   utils
