# _dev_mega_0series.py
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
            'PORTB',
            'PORTC',
            'PORTD',
            'PORTE',
            'PORTF',
            'PORTMUX',
            'RTC',
            'TCA0',
            'TCB0',
            'TCB1',
            'TCB2',
            'TCB3',
            'VREF',
            'ADC0',
            'ACP0',
            'USART0',
            'USART1',
            'USART2',
            'USART3',
            'SPI0',
            'TWI0',
            'FUSES',
            'USERROW'
        ]

        builder.build_peripherals(self, peripherals)
