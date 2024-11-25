IDE Integration
===============

PlatformIO
**********

https://platformio.org/


*yasimavr* can be integrated to PlatformIO and used as a custom debugging tool.


Examples
--------

Example of *platformio.ini* to debug a firmware for Arduino Uno, using *yasimavr* in command line :

.. code-block::

    [env:uno]
    platform = atmelavr
    board = uno
    framework = arduino
    debug_tool = custom
    debug_port = localhost:1234
    debug_server =
        python
        -m
        yasimavr
        -g
        1234
        -m
        atmega328p
        -f
        16000000
        -t
        pin
        PB5
        -o
        ${PROJECT_DIR}/trace.vcd
        ${platformio.build_dir}/${this.__env__}/firmware.elf


To use a simulation script, *platformio.ini* should look like this example below :

.. code-block::

    [env:uno]
    platform = atmelavr
    board = uno
    framework = arduino
    debug_tool = custom
    debug_port = localhost:1234
    debug_server =
        python
        ${PROJECT_DIR}/sim_script.py
