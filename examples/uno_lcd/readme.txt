uno_lcd:
The device is an ATmega328 programmed with the Arduino Uno core.
The firmware reads a text stored in a external serial EEPROM, using the I2C interface.
The text is output to a 16x2 LCD display using the HD44780 protocol, the text is scrolled from right to left.
The firmware uses the standard Arduino library LiquidCrystal and Wire.


Additional prerequisites to run the example :
- Arduino IDE v2.3.x or later (downloadable here: https://www.arduino.cc/en/software)
- PyQt6 (downloadable here: https://pypi.org/project/PyQt6)


Instructions :
- Load the firmware in the Arduino IDE (or CLI), select the Arduino Uno board and compile/export
the binaries.
- Copy the compiled binaries (ELF format) in the same directory as the example script "uno_lcd.py".
- The binaries should be named "uno_lcd.ino.elf"
- Launch the example script with Python.

To stop the script, simply close the LCD window.

The serial EEPROM is loaded with the content of the file "eeprom.txt"
The text is extracted from A Tale of Two Cities from Charles Dickens (because why not) but it may be replaced by any other text.
