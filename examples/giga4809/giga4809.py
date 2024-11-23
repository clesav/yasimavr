from yasimavr import device_library
from yasimavr.lib.core import SimLoop, Firmware
from yasimavr.utils.sim_dump import sim_dump

#Create a bespoke device descriptor from the custom YAML file
desc = device_library.DeviceDescriptor.create_from_file('atgiga4809.yml')

#Build the device model using the bespoke descriptor
dev = device_library.load_device_from_config(desc)

#Create a simulation loop, an accessor and load the test firmware
simloop = SimLoop(dev)
dev_acc = device_library.DeviceAccessor(dev)

fw = Firmware.read_elf('../mega4809_blink/mega4809_blink_fw.elf')
fw.frequency = 1000000
dev.load_firmware(fw)

#Iterate over all the GPIO port,set all pins to output and set the output value
#to a value unique for each, port A = 0, portB = 1, portC= 2, etc...
for i, p in enumerate(('A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N')):
    acc = getattr(dev_acc, 'PORT' + p)
    acc.DIR = 0xFF
    acc.OUT = i

#Run the firmware until it stops.
simloop.run()

#Dump the simulation state. The pin state Low/High should match the value set in the
#for loop above.
sim_dump(simloop, open('dump.txt', 'w'))
