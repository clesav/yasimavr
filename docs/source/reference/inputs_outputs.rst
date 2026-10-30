=====================
Inputs / Outputs APIs
=====================

This section describes the APIs used for the various types of input/outputs simulated
with `yasimavr` other than using regular GPIO.


.. sip:specification:: yasimavr.lib.core


.. _ADC_ref:

Analog-to-Digital Converters
----------------------------


.. sip:namespace:: ADC

   The namespace ADC regroups enumerations used in ADC peripheral configuration
   and signalling

   .. sip:enum:: ADC.Channel


   .. sip:class:: ADC.channel_config_t
      :members:


   .. sip:class:: ADC.reference_config_t
      :members:


   .. sip:enum:: ADC.SignalId


.. _ACP_ref:

Analog Comparators
------------------


.. sip:namespace:: ACP

   The namespace ACP regroups enumerations used in ACP/AC peripheral configuration
   and signalling

   .. sip:enum:: ACP.Channel


   .. sip:class:: ACP.channel_config_t
      :members:


   .. sip:enum:: ACP.SignalId


.. _SPI_ref:

Serial Peripheral Interface
---------------------------


.. sip:namespace:: SPI

   .. seealso::
      Utilities for simulation scripts are defined in the module :py:mod:`yasimavr.utils.spi`.

   .. sip:enum:: SPI.SerialMode


   .. sip:enum:: SPI.BitOrder


   .. sip:enum:: SPI.Line


   .. sip:class:: SPI.EndPoint
      :members: undoc


.. _TWI_ref:

Two Wire Interface
------------------

This API (tentatively) defines a simulation of a TWI (a.k.a. I2C or SMBus) implementation.

It supports multiple hosts/clients, arbitration and bus collision detections. However,
it is not multi-thread safe.

It is implemented by 3 classes:

- EndPoint is an abstract interface defining a generic device connected to a TWI bus.
- Client : Basic state machine & transitions for a client side interface.
- Host : Basic state machine & transitions for a host side interface.

These classes are abstract, the way classes access the SCL and SDA line must be defined by
reimplementation by overriding `set_line_state()` and using `line_state_changed()`.

Client and Host are designed to be controlled by an upper level object (a controller).
The controller gets notified or events on the bus by signalling, and shall use the interface API
to react accordingly.

.. seealso::
   Utilities for simulation scripts are defined in the module :py:mod:`yasimavr.utils.twi`.


.. sip:namespace:: TWI


   .. sip:enum:: TWI.Line


   .. sip:enum:: TWI.BusState


   .. sip:enum:: TWI.SignalId


   .. sip:class:: TWI.EndPoint
      :members: undoc


   .. sip:class:: TWI.Host
      :members: undoc


   .. sip:class:: TWI.Client
      :members: undoc


.. _UART_ref:

Universal Asynchronous Serial Interface
---------------------------------------


.. sip:namespace:: UART


   .. seealso::
      Utilities for simulation scripts are defined in the module :py:mod:`yasimavr.utils.uart`.


   .. sip:enum:: UART.SignalId


   .. sip:enum:: UART.ClockMode


   .. sip:enum:: UART.Line


   .. sip:enum:: UART.Parity


   .. sip:class:: UART.USART
      :members: undoc

