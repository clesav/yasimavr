mega328_stopwatch:
An ATmega328 is programmed to measure a button press duration in 10th of secs.
A push button is connected to PB8 and drives it to ground when pushed.

To compile the firmware, ensure AVR-GCC is in PATH and execute :
avr-gcc -Os -g -mmcu=atmega328 -o mega328_stopwatch_fw.elf mega328_stopwatch_fw.c
