mega4809_blink:
Same example as mega328_blink but with an ATmega4809

To compile the firmware, ensure AVR-GCC is in PATH and execute :
avr-gcc -Os -g -mmcu=atmega4809 -o mega4809_blink_fw.elf mega4809_blink_fw.c
