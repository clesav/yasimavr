# dev_mega_0series.py
#
# Copyright 2023 Clement Savergne <csavergne@yahoo.com>
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
This module initialises a ATmega4809
'''

from ...lib import arch_m0 as _archlib
from ._builders_arch_xt import XT_DeviceBuilder

#========================================================================================
#Device class definition

class dev_mega_0series(_archlib.AVR_ArchMega0_Device):

    def __init__(self, model, builder):
        super().__init__(builder.get_device_config())

        peripherals = [
            'CPUINT',
            'SLPCTRL',
            'CLKCTRL',
            'RSTCTRL',
            'NVMCTRL',
            'MISC',
            'PORTA',
            'PORTC',
            'PORTD',
            'PORTF',
            'PORTMUX',
            'RTC',
            'TCA0',
            'TCB0',
            'TCB1',
            'TCB2',
            'VREF',
            'ADC0',
            'ACP0',
            'USART0',
            'USART1',
            'USART2',
            'SPI0',
            'TWI0',
            'FUSES',
            'USERROW'
        ]

        if model in ('atmega809', 'atmega1609', 'atmega3209', 'atmega4809'):
            peripherals.extend(['PORTB', 'PORTE', 'TCB3', 'USART3'])

        builder.build_peripherals(self, peripherals)


def device_factory(model):
    return XT_DeviceBuilder.build_device(model, dev_mega_0series)
