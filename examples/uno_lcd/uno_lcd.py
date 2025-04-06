import os
import threading
from yasimavr.device_library import load_device
from yasimavr.lib import core as corelib
from yasimavr.utils.parts.lcd_hd44780 import HD44780_Display, HD44780_Driver, HD44780_Interface4Bits
from yasimavr.utils.parts.paged_eeprom import PagedEEPROM
from PyQt6 import QtCore, QtGui, QtWidgets

#LCD pin mapping
#LCD :            D4     D5     D6     D7     EN     RS
#MCU :            PD5    PD4    PD3    PD2    PB3    PB4
#Arduino Uno :    5      4      3      2      11     12

#I2C pin mapping
#EEPROM :         SCL    SDA
#MCU :            PC5    PC4


LCD_PIN_NAMES = ['PD5', 'PD4', 'PD3', 'PD2', 'PB3', 'PB4']
I2C_PIN_NAMES = ['PC5', 'PC4']


def main():

    #Create a new MCU model ATMega328
    device = load_device('atmega328')

    #Create a asynchronous simulation loop for this device
    simloop = corelib.AsyncSimLoop(device)

    #Load the firmware with a MCU clock of 16MHz
    fw_path = os.path.join(os.path.dirname(__file__), 'uno_lcd.ino.elf')
    firmware = corelib.Firmware.read_elf(fw_path)
    firmware.frequency = 16000000
    device.load_firmware(firmware)

    #Pin to which the LED will be connected
    lcd_pins = [device.find_pin(pn) for pn in LCD_PIN_NAMES]

    #Qt initialisation
    qt_app = QtWidgets.QApplication([])
    window = QtWidgets.QMainWindow()
    mainWidget = QtWidgets.QWidget()
    window.setCentralWidget(mainWidget)

    lcd_driver = HD44780_Driver()
    lcd_display = HD44780_Display(lcd_driver, 16, 2, window)
    lcd_interface = HD44780_Interface4Bits(lcd_driver, lcd_pins)

    layout = QtWidgets.QVBoxLayout(mainWidget)
    layout.addWidget(lcd_display)

    window.show()

    #External eeprom creation and initialisation
    #Add a null character at the end as a EOF marker
    text = open('eeprom.txt', 'r').read().encode('ascii') + b'\0'
    eeprom = PagedEEPROM(5, 256, 1024, 1000, text)
    i2c_pins = [device.find_pin(pn) for pn in I2C_PIN_NAMES]
    eeprom.scl_wire.attach(i2c_pins[0])
    eeprom.sda_wire.attach(i2c_pins[1])
    eeprom.init(simloop.cycle_manager())

    #Start a separate thread to run the simulation loop
    simloop_thread = threading.Thread(target=simloop.run)
    simloop_thread.start()

    #The loop is initialised in the Stopped state so the first thing
    #to do is to order it to run continuously
    with simloop:
        simloop.loop_continue()

    #Enter the main GUI event loop of Qt
    qt_app.exec()

    #On leaving the application, stop the simloop
    #The thread will terminate.
    with simloop:
        simloop.loop_kill()


if __name__ == '__main__':
    main()
