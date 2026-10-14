# dev_tiny_1series.py
#
# Copyright 2026 Clement Savergne <csavergne@yahoo.com>
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
This module initialises a device model for the ATtiny 1-series:
attiny212/214/412/414/814/816/817/1614/1616/1617/3216/3217
'''

from ._builders_arch_xt import XT_DeviceBuilder, XT_BaseDevice
from ..descriptors import DeviceDescriptor
import re

#========================================================================================
#Device class definition

class dev_tiny_1series(XT_BaseDevice):

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
            'PORTB',
            'PORTC',
            'PORTMUX',
            'RTC',
            'TCA0',
            'TCB0',
            'TCD0',
            'WDT',
            'VREF',
            'ADC0',
            'ACP0',
            'DAC0',
            'USART0',
            'SPI0',
            'TWI0',
            'FUSES',
            'USERROW'
        ]

        m = re.fullmatch("[a-zA-Z]+([0-9]+)", dev_descriptor.name)
        dev_num = m.group(1)

        if int(dev_num[-1]) > 2:
            peripherals.append('PORTB')
        if int(dev_num[-1]) > 4:
            peripherals.append('PORTC')
        if int(dev_num) >= 1600:
            peripherals.extend(('TCB1', 'ADC1', 'ACP1', 'ACP2', 'DAC1', 'DAC2'))

        builder.build_peripherals(self, peripherals)


    def arch_init(self):
        #self._builder_.add_pin_driver_mux_configs(self, 'TCA0') #TODO
        self._builder_.add_pin_driver_mux_configs(self, 'TCB0')
        if 'TCB1' in self._descriptor_.peripherals:
            self._builder_.add_pin_driver_mux_configs(self, 'TCB1')
        self._builder_.add_pin_driver_mux_configs(self, 'USART0')
        self._builder_.add_pin_driver_mux_configs(self, 'SPI0')
        self._builder_.add_pin_driver_mux_configs(self, 'TWI0')

        return True


def device_factory(model):
    dev_desc = DeviceDescriptor.create_from_model(model)
    return XT_DeviceBuilder.build_device(dev_desc, dev_tiny_1series)
