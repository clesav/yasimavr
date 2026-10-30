=====================
Core library (basics)
=====================

This section describes the key classes common to all device architecture
and essentials to simulation scripts.
They are accessible by the following import :

.. code-block:: py

   import yasimavr.lib.core


.. sip:specification:: yasimavr.lib.core


Key objects and classes
-----------------------

The following 4 classes are used in every simulation script.


.. sip:class:: Device
   :members:


.. sip:class:: Firmware
   :members:


.. sip:class:: SimLoop
   :members:


.. sip:class:: AsyncSimLoop
   :members:


Wire/Pin classes
----------------

The Wire/Pin classes provide a crude simulation of the behaviour of external I/O pins
of a micro-controller and provide a way for the MCU model to interact with the
simulated "outside world" by mean of discrete I/Os or bus interfaces. (SPI, TWI, UART etc.)

To get an idea how to use them, see the examples `uno_lcd` and `mega328_stopwatch`.


.. important::
   `yasimavr` in not an electrical circuit simulator nor does it aim to be.
   Much better tools are available out there for that purpose.


.. sip:class:: Wire
   :members: undoc


.. sip:class:: Pin
   :members:


Logging
-------

The Logger class provides the facilities required for log output during simulation runs
Default logging is output to stdout but an alternative LogWriter can be specified to output to
a file or elsewhere.


.. sip:class:: Logger
   :members: undoc


Signal/Hook classes
-------------------

See :ref:`signal_concept` concept section for a description of the signal/hook framework


.. sip:class:: Signal
   :members:


.. sip:class:: DataSignal
   :members:


.. sip:class:: SignalHook
   :members:


.. sip:class:: CallableSignalHook
   :members:


Simulation time/cycle management classes
----------------------------------------

These classes provide the framework to count the simulation cycles and manage
timers, i.e. generic object that are called back at a specified cycle.

They also act as a facility to manage the various clock sources and track the
simulated time vs. cycle.


.. sip:class:: CycleTimer
   :members:


.. sip:class:: CallableCycleTimer
   :members:


Device internals access
-----------------------

.. sip:class:: NonVolatileMemory
   :members:


.. sip:class:: DeviceDebugProbe
   :members:

