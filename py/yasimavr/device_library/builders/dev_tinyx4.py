# dev_tinyx4.py
#
# Copyright 2025 Clement Savergne <csavergne@yahoo.com>
#
# This file is part of yasim-avr.
#
# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.

'''
This module initialises a ATtinyx4
'''

from ._builders_arch_avr import AVR_DeviceBuilder, AVR_BaseDevice
from ..descriptors import DeviceDescriptor

#========================================================================================
#Device class definition

class dev_tinyx4(AVR_BaseDevice):

    def __init__(self, dev_descriptor, builder):
        super().__init__(dev_descriptor, builder)

        peripherals = [
            'CPUINT',
            'RSTCTRL',
            'SLPCTRL',
            'FUSES',
            'NVMCTRL',
            'MISC',
            'PORTA',
            'PORTB',
            'EXTINT',
            'TC0',
            'TC1',
            'ADC',
            'ACP',
            'VREF',
            'USI',
            'WDT',
        ]

        builder.build_peripherals(self, peripherals)


    def arch_init(self):
        self._builder_.add_pin_driver_mux_configs(self, 'USI')

        return True

def device_factory(model):
    dev_desc = DeviceDescriptor.create_from_model(model)
    return AVR_DeviceBuilder.build_device(dev_desc, dev_tinyx4)
