import os
from yasimavr.device_library import load_device
from yasimavr.lib import core as corelib


#This function implements a hook that will be called by the pin signal.
#It captures digital state change notifications and prints the new LED state.
def pin_hook(sigdata, hooktag):
    #Filter the notifications, we're only interested in digital state changes
    if sigdata.sigid == corelib.Wire.SignalId.DigitalChange:
        #Obtain the new pin state
        pin_state = sigdata.data.as_uint()
        led_state = 'ON' if pin_state else 'OFF'
        #Print the state
        print('LED state:', led_state)


def main():

    #Uncomment this to have the global trace and logging
    #corelib.global_logger().set_level(corelib.Logger.Level.Trace)

    #Create a new MCU model ATMega328
    device = load_device('atmega328')

    #Uncomment this to have the device trace and logging
    #device.logger().set_level(corelib.Logger.Level.Trace)
    #device.find_peripheral('TC_1').logger().set_level(corelib.Logger.Level.Trace)

    #Create a simulation loop for this device
    simloop = corelib.SimLoop(device)

    #Load the firmware with a MCU clock of 1MHz
    fw_path = os.path.join(os.path.dirname(__file__), 'mega328_blink_fw.elf')
    firmware = corelib.Firmware.read_elf(fw_path)
    firmware.frequency = 1000000
    device.load_firmware(firmware)

    #Find the pin driving the LED and connect a hook to its signal
    pin = device.find_pin('PB0')
    hook = corelib.CallableSignalHook(pin_hook)
    pin.signal().connect(hook)

    #Run the simulation for 10 million cycles (representing 10 secs of simulated time).
    #Because the fast simulation mode is disabled, this should be roughly 10 secs of
    #real world time as well.
    simloop.run(10000000)


if __name__ == '__main__':
    main()
