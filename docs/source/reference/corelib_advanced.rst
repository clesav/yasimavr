=======================
Core library (advanced)
=======================

This section contain classes that are not commonly used in simulation scripts.
They are available for advanced access to device model internals or to define
custom device or peripheral models.


.. sip:specification:: yasimavr.lib.core


Base numeric types and classes
------------------------------

.. sip:class:: reg_addr_t
    :members: undoc


.. sip:class:: sim_id_t
    :members: undoc,special


Bit/byte manipulation classes
-----------------------------

These classes are mostly used for bit masking and
8-bits register definitions & value manipulation


.. sip:class:: bitmask_t
   :members:


.. sip:class:: regmask_t
   :members:


.. sip:class:: bitspec_t
   :members:


.. sip:class:: regbit_t
   :members:


.. sip:class:: regbit_compound_t
   :members:


GPIO management
---------------

.. sip:class:: PinDriver
   :members: all


.. sip:class:: PinManager
   :members: all


Peripheral implementation
-------------------------

.. sip:class:: Peripheral
   :members: all


.. sip:class:: IORegister
   :members: all


.. sip:class:: IORegHandler
   :members: all


.. sip:class:: ioreg_write_t
   :members: all


.. sip:class:: NVM_request_t
   :members: all


Interrupt management
--------------------

.. _core_intctrl_ref:

.. sip:class:: InterruptController
   :members: all


.. sip:class:: InterruptHandler
   :members: all


.. sip:class:: AbstractInterruptFlag
   :members: all


.. sip:class:: InterruptFlag
   :members: all


Logging API
-----------

.. sip:class:: LogWriter
   :members: all


.. sip:class:: LogHandler
   :members: all


Model internal classes
----------------------

.. sip:class:: DeviceConfiguration
   :members: all


.. sip:class:: CoreConfiguration
   :members: all


.. sip:class:: Core
   :members: all


.. _core_slpctrl_ref:

.. sip:class:: SleepController
   :members: all


.. sip:class:: DummyController
   :members: all


.. sip:class:: MemorySectionManager
   :members: all


.. sip:class:: CycleManager
   :members: all
