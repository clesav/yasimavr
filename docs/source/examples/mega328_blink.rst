LED Blink (ATMega328)
=====================

This example illustrates the classic "ATMega328 running a firmware to blink a LED" example.
It is composed of two files:

* mega328_blink_fw.c : source code for the firmware to run inside the model
* mega328_blink.py : simulation script

The Firmware uses the GPIO pin PB0 as output to drive a (virtual) LED.

It uses the Timer1 configured in CTC mode to trigger overflow interrupts at a 0.5Hz frequency.
On each interrupt, the pin PB0 output state is toggled, thus giving it a 1 Hz period, 50% duty cycle.

The simulation script reads the firmware binary file and loads it into a ATMega328 model.
It connects a SignalHook that simply prints out the state of the LED whenever it changes.
Finally, it runs the simulation for 10 secs of simulated time.


Firmware source
---------------

mega328_blink.fw.c:

.. literalinclude:: /../../examples/mega328_blink/mega328_blink_fw.c
   :language: c


Simulation script
-----------------

mega328_blink.py:

.. literalinclude:: /../../examples/mega328_blink/mega328_blink.py
   :language: py


.. note::
   The script assumes that the user has compiled the firmware code into a ELF file named *mega328_blink_fw.elf* in the same directory.
