================
Python Utilities
================


VCD Output
----------

.. automodule:: yasimavr.utils.vcd_recorder


.. autoclass:: VCD_Recorder
   :members:


.. autoclass:: Formatter
   :members:


Simulation State Dump
---------------------

.. automodule:: yasimavr.utils.sim_dump


.. autofunction:: sim_dump


GDB Server
----------

.. automodule:: yasimavr.utils.gdb_server


.. autoclass:: GDB_Stub


SPI
---

.. automodule:: yasimavr.utils.spi

.. autoclass:: SPISimpleClient
   :members:

.. autoclass:: SPISimpleHost
   :members:


TWI
---

.. automodule:: yasimavr.utils.twi

.. autoclass:: TWISimpleClient
   :members:


UART
----

.. automodule:: yasimavr.utils.uart

.. autoclass:: UART_IO
   :members: write, readinto

.. seealso::
   For the full list of members, refer to the Python standard documentation for io.RawIOBase.

.. autoclass:: RawUART
   :members:

.. seealso::
   For the full list of members, see :py:class:`yasimavr.lib.core.UART.USART`.
