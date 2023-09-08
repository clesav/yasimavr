mega328_blink:
An ATmega328 is programmed to drive a LED at a frequency of 1Hz.
The timing is managed by Timer1.

To compile the firmware, ensure AVR-GCC is in PATH and execute :
avr-gcc -Os -g -mmcu=atmega328 -o mega328_blink_fw.elf mega328_blink_fw.c
