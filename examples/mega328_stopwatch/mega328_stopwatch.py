'''
This example is a stop watch implemented with a ATMega328.
It displays a 7-segment display with 2 digits and a push button
The push-button is connected to PB7 of the MCU and connects it to ground
when pressed.
When the button is pressed, the MCU counts by 10th of seconds and
outputs the value in binary form on GPIO pins PB0 to PB6.
It stops counting when the button is released or when the maximum value
of 99 is reached.
'''

import os
import threading
from yasimavr.device_library import load_device
from yasimavr.lib import core as corelib
from PyQt5 import QtWidgets, Qt

global simloop
simloop = None

class Number(QtWidgets.QLCDNumber):
    
    #Is it necessary that the led state changes transit via a Qt signal
    #because the notification is raised in a thread different to the main
    #Qt application thread. The Qt signal/slot system takes care of the
    #synchronisation
    sig_pin_changed = Qt.pyqtSignal(list)
    
    def __init__(self, parent, device, port_name):
        '''
        Initialisation of a LCD number widget with 2 digits.
        parent : the Qt window containing the widget
        device : a AVR_Device whose pins the LCD connects to.
        port_name : name of the GPIO port of the device to connect to
        '''
        
        #Initialise the QLCDNumber instance with 2 digits
        super().__init__(2, parent)
        
        #Obtain a list of the 7 first pins of the port given in argument 
        self.pins = [device.find_pin('P' + port_name + str(i)) for i in range(7)]
        
        #Create a hook and connect it to the signal of each pin in the list above
        self.hook = corelib.AVR_CallableSignalHook(self._pin_signal_raised)
        for i, pin in enumerate(self.pins):
            pin.signal().connect_hook(self.hook, i)
        
        #List storing the value given by the pin state
        self.pin_values = [0] * len(self.pins)
        
        #Connect our signal to our slot for synchronisation
        self.sig_pin_changed.connect(self._slot_pin_changed)
    
    
    #Callback called on a signal raise from any of the 7 output pins
    #The pin_index value (0 to 7) gives the pin that actually raised the signal
    def _pin_signal_raised(self, sigdata, pin_index):
        #Filter on digital state changes
        if sigdata.sigid == corelib.AVR_Pin.SignalId.DigitalStateChange:
            
            #Store the new pin state, extracted from the signal data
            if sigdata.data.as_uint() == corelib.AVR_Pin.State.High:
                self.pin_values[pin_index] = 1
            else:
                self.pin_values[pin_index] = 0
            
            #Emit the signal to indicate a change of value
            self.sig_pin_changed.emit(self.pin_values)
    
    
    #Slot receiving the pin state changes
    def _slot_pin_changed(self, pin_values):
        
        #Compile the bits from the 7 pins into a single integer
        bin_value = 0
        for i, v in enumerate(pin_values):
            bin_value += v << i
        
        #Display the binary value (replace it by a double dash if the value
        #is over the limit)
        if bin_value >= 100:
            self.display('--')
        else:
            self.display(bin_value)


class Button(QtWidgets.QPushButton):

    def __init__(self, parent, pin):
        super().__init__('Start/Stop', parent)
        self.pin = pin
        #Connect the signals from the push-button
        #A press will set the pin low
        self.pressed.connect(self.press_button)
        #A release will set the pin floating
        self.released.connect(self.release_button)
    
    def press_button(self):
        with simloop:
            self.pin.set_external_state(corelib.AVR_Pin.State.Low)
            #simloop.loop_continue()
    
    def release_button(self):
        with simloop:
            self.pin.set_external_state(corelib.AVR_Pin.State.Floating)
            #simloop.loop_continue()


def main():
    
    #Uncomment this to have the device trace
    #corelib.AVR_StandardLogger.set_level(corelib.AVR_DeviceLogger.LOG_TRACE)
    
    #Create a new MCU model ATMega328
    device = load_device('atmega328')
    
    #Create a asynchronous simulation loop for this device
    global simloop
    simloop = corelib.AVR_AsyncSimLoop(device)
    
    #Load the firmware with a MCU clock of 16MHz
    fw_path = os.path.join(os.path.dirname(__file__), 'mega328_stopwatch_fw.elf')
    firmware = corelib.AVR_Firmware.read_elf(fw_path)
    firmware.frequency = 16000000
    device.load_firmware(firmware)
    
    #Pin to which the button will be connected
    pin_button = device.find_pin('PB7')
    
    #GUI construction
    qt_app = QtWidgets.QApplication([])
    window = QtWidgets.QMainWindow()
    window.resize(300, 300)
    mainWidget = QtWidgets.QWidget()
    window.setCentralWidget(mainWidget)
    
    number = Number(window, device, 'B')
    button = Button(window, pin_button)
    
    layout = QtWidgets.QVBoxLayout(mainWidget)
    layout.addWidget(number)
    layout.addWidget(button)
    
    window.show()
    
    #Start a separate thread to run the simulation loop
    simloop_thread = threading.Thread(target=simloop.run)
    simloop_thread.start()
    
    #The loop is initialised in the Stopped state so the first thing
    #to do is to order it to run continuously
    with simloop:
        simloop.loop_continue()
    
    #Enter the main GUI event loop of Qt
    qt_app.exec_()
    
    #On leaving the application, stop the simloop
    #The thread will terminate.
    with simloop:
        simloop.loop_kill()


if __name__ == '__main__':
    main()
