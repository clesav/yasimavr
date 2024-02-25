Device Library Management
=========================

Introduction
------------

The device library provides the means for loading and configuring a simulation model for a particular MCU model or variant.
The information is also used for accessing the device internals in a 'pythonic' and human-readable way, mainly for testing and debugging purposes.
The device library for yasimavr is built around 3 categories of classes/objects :

* Descriptors : contains all the information describing a device: pins, interrupts, memory layout, registers, etc.
* Builders : factory classes to instantiate and configure a device simulation model based on the information stored in descriptors.
* Accessors : allows to access the internals of a device model, in particular the registers in a 'Pythonic' way. Designed primarily for automatic testing of simulation models but may also be used in simulation.


The information for configuring device models is stored in configuration files using the YAML format.

`yasimavr` contains pre-defined models for some device variants. To use these variants, there is no need for a user to dive into the device library package and configuration files, and one can simply use `device_library.load_device()`.
The list of pre-defined models can be obtained in command line by executing ``python -m yasimavr --list-models``


.. note::
   Using the device library to instantiate a simulation model is not compulsory, it only makes it more convenient for commonly used device models.
   It is possible to build and configure a model from scratch using the C++ API classes if one feels like it...


Device Descriptor
-----------------

A descriptor is a Python object storing the information required to build a simulated device model.
There are two ways to load a Device object : by name or by file.

The information is loaded from device configuration files in YAML format.


Reg Path format
***************

In device configuration files, registers can be referenced to by what is called a 'reg path'.
A reg path is a string representing a path through a device description tree to a register or a field.
It can be absolute (from the root) or relative (from a given peripheral instance).

Valid formats for the regpath are:

* [P]/[R]/[F] (1)
* [R]/[F]     (2)
* [A]/[F]     (3)
* [P]/[R]     (4)
* [R]         (5)

where [P] is a peripheral instance name, [R] a register name, [A] a register address,
[F] a field or a combination of fields using the character '|'.

Register addresses can be decimal or hexadecimal.
Fields can be represented by a name, a bit number X or a bit range X-Y. However, if the register is given by an address, named fields are not allowed.

Examples:

* `SLPCTRL/SMCR/SE` : absolute path to the bit SE of the SMCR register belonging to the Sleep Controller
* `CPU/GPIOR0` : absolute path to the GPIOR0 register
* `SPCR/SPE` : relative path to the SPI Enable bit of the SPI Control Register (valid in the context of a SPI interface controller)
* `EXTINT/PCICR/2` : absolute path to bit 2 of the PCICR register
* `EXTINT/PCMSK0/0-3` : absolute path to bits 0 to 3 of the PCMSK0 register
* `0x49/5|7` : path to the bits 5 and 7 of hexadecimal address 0x49
* `2/0` : path to bit 0 of decimal address 2

.. note::
    | A list of path elements can be given as a regpath and is parsed the same way.
      For example, ['USART', 'UCSRB', 'RXEN'] is equivalent to 'USART/UCSRB/RXEN'.

    | In some cases, registers may be split across multiple peripherals.
      For example in ATMega328, MCUCR is split between SLPCTRL (fields BODSE, BODS)
      and CPUINT (fields IVCE, IVSEL). In this case, SLPCTRL does not recognize CPUINT
      fields and vice-versa. However, fields are always accessible by bit number.
      For example, SLPCTRL/MCUCR/IVSEL is invalid but SLPCTRL/MCUCR/0 is valid.


Reference (Descriptor structure)
********************************

.. automodule:: yasimavr.device_library.descriptors


.. autoclass:: DeviceDescriptor
   :members:


.. autoclass:: PeripheralInstanceDescriptor
   :members:


.. autoclass:: PeripheralClassDescriptor


.. autoclass:: ProxyRegisterDescriptor


.. autoclass:: RegisterDescriptor
   :members:


.. autoclass:: RegisterFieldDescriptor
   :members:


.. autoclass:: InterruptMapDescriptor


.. autoclass:: MemorySpaceDescriptor


.. autoclass:: MemorySegmentDescriptor


Reference (Utility functions)
*****************************

.. autofunction:: convert_to_regbit


.. autofunction:: convert_to_regbit_compound


.. autofunction:: convert_to_bitmask


Device Builders
---------------

.. autoclass:: yasimavr.device_library.builders._base.DeviceBuilder
    :members:


.. autoclass:: yasimavr.device_library.builders._builders_arch_avr.AVR_DeviceBuilder


.. autoclass:: yasimavr.device_library.builders._builders_arch_avr.AVR_BaseDevice


.. autoclass:: yasimavr.device_library.builders._builders_arch_xt.XT_DeviceBuilder


.. autoclass:: yasimavr.device_library.builders._builders_arch_xt.XT_BaseDevice


Accessors
---------

The core library provides a DeviceDebugProbe class that allows to read and write to
I/O registers as if the probe was the CPU. However the probe only deals with 8-bits
raw byte values.

Accessors provide an upper layer by using the information contained in descriptors, from
device configuration files, and provide two features:

* A fine-grained access to individual I/O bits and fields,
* Pretty printing and human-readable interpretation of values for read/write operations.

For example, the following (using the debug probe object to enable sleep):

.. code-block:: python

   value = probe.read_ioreg(0x33)  #reading the register SMCR
   value |= 1                      #setting SE (bit 0) to 1
   probe.write_ioreg(0x33, value)  #writing back to SMCR

is equivalent, using the accessor system, to :

.. code-block:: python

   device_accessor.SLPCTRL.SMCR.SE = 'enabled'


The following (Reading the current sleep mode):

.. code-block:: python

   value = probe.read_ioreg(0x33)  #reading the register SMCR
   mode = (value >> 1) & 0x07      #Extracting the SM field
   print('Sleep mode :', mode)     #Print the value (raw)

will output: ``>>> Sleep mode : 0``

The equivalent, using the accessor system :

.. code-block:: python

   print('Sleep mode :', device_accessor.SLPCTRL.SMCR.SM)

will output: ``>>> Sleep mode : IDLE``


Reference (Accessor structure)
********************************


.. autoclass:: yasimavr.device_library.accessors.DeviceAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors.PeripheralAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors.RegisterAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors._FieldAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors.BitFieldAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors.IntFieldAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors.EnumFieldAccessor
    :members:


.. autoclass:: yasimavr.device_library.accessors.RawFieldAccessor
    :members:
