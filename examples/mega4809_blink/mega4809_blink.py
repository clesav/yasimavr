import os
import threading
from yasimavr.device_library import load_device
from yasimavr.lib import core as corelib
from PyQt5 import QtCore, QtGui, QtWidgets


class LED(QtWidgets.QWidget):
    '''
    This class defines a widget drawing a LED whose state is driven
    by a pin of the MCU.
    The class connects a hook to the pin signal and listen for pin
    state changes. When it happens, it repaints the LED, represented
    by a filled circle.
    '''

    #Is it necessary that the led state changes transit via a Qt signal
    #because the notification is raised in a thread different to the main
    #Qt application thread. The Qt signal/slot system takes care of the
    #synchronisation
    sig_pin_changed = QtCore.pyqtSignal(bool)


    def __init__(self, parent, pin):
        '''
        Initialisation of a LED widget.
        parent : the Qt window containing the widget
        pin : a AVR_Pin object to which the LED connects.'''

        super().__init__(parent)

        #Create a hook and connect it to the pin
        self.hook = corelib.CallableSignalHook(self._pin_signal_raised)
        pin.signal().connect(self.hook)
        #Default state: OFF
        self.state = False
        #Connect our signal to our slot for synchronisation
        self.sig_pin_changed.connect(self._slot_pin_changed)


    def _pin_signal_raised(self, sigdata, _):
        #Filter on digital state changes
        if sigdata.sigid == corelib.Pin.SignalId.DigitalChange:
            #Obtain the new pin state and emit the widget signal
            pin_state = sigdata.data.as_uint()
            #turn on the LED only if the pin is driven high
            led_state = (pin_state == corelib.Pin.State.High)
            self.sig_pin_changed.emit(led_state)


    #Slot receiving the LED state changes. Store the new state ON/OFF
    #and call for a repaint
    def _slot_pin_changed(self, state):
        self.state = state
        self.repaint()


    #Override of QWidget to draw the LED
    def paintEvent(self, event):
        #Draw a filled circle of 40 pixel diameter in the center of the widget area
        #The filling color is gray if the led is OFF and red if ON.
        painter = QtGui.QPainter(self)
        painter.setBrush(QtCore.Qt.red if self.state else QtCore.Qt.gray)
        painter.drawEllipse(self.geometry().center(), 40, 40)
        #The rest of the painting is left to Qt
        super().paintEvent(event)


def main():

    #Create a new MCU model ATMega4809
    device = load_device('atmega4809')

    #Uncomment this to have the device trace
    #device.logger().set_level(corelib.Logger.Level.Trace)

    #Create a asynchronous simulation loop for this device
    simloop = corelib.AsyncSimLoop(device)

    #Load the firmware with a MCU clock of 1MHz
    fw_path = os.path.join(os.path.dirname(__file__), 'mega4809_blink_fw.elf')
    firmware = corelib.Firmware.read_elf(fw_path)
    firmware.frequency = 1000000
    device.load_firmware(firmware)

    #Pin to which the LED will be connected
    pin = device.find_pin('PB0')

    #Qt initialisation
    qt_app = QtWidgets.QApplication([])
    window = QtWidgets.QMainWindow()
    window.resize(400,400)
    widget = LED(window, pin)
    window.setCentralWidget(widget)
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
