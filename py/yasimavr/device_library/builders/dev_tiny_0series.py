# dev_tiny_0series.py
#
# Copyright 2024 Clement Savergne <csavergne@yahoo.com>
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
This module initialises a device model for the ATtiny 0-series:
attiny202/4
attiny402/4/6
attiny804/6/7
attiny1604/6/7
'''

from ._builders_arch_xt import XT_DeviceBuilder, XT_BaseDevice
from ..descriptors import DeviceDescriptor

#========================================================================================
#Device class definition

class dev_tiny_0series(XT_BaseDevice):

    def __init__(self, dev_descriptor, builder):
        super().__init__(dev_descriptor, builder)

        peripherals = [
            'CPUINT',
            'SLPCTRL',
            'CLKCTRL',
            'RSTCTRL',
            'NVMCTRL',
            'MISC',
            'PORTA',
            'PORTMUX',
            'RTC',
            'TCA0',
            'TCB0',
            'VREF',
            'ADC0',
            'ACP0',
            'USART0',
            'SPI0',
            'TWI0',
            'FUSES',
            'USERROW'
        ]

        if dev_descriptor.name[-1] != '2':
            peripherals.append('PORTB')

        if dev_descriptor.name[-1] in ('6', '7'):
            peripherals.append('PORTC')

        builder.build_peripherals(self, peripherals)


def device_factory(model):
    dev_desc = DeviceDescriptor.create_from_model(model)
    return XT_DeviceBuilder.build_device(dev_desc, dev_tiny_0series)
