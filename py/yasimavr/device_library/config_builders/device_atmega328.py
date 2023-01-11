# device_atmega328.py
#
# Copyright 2021 Clement Savergne <csavergne@yahoo.com>
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
This module initialises a ATmega328
'''

from ...lib import core as _corelib
from ...lib import arch_avr as arch_lib
from ..descriptors import DeviceDescriptor
from .convertors_avr_xx8 import *

DEV_NAME = 'atmega328'

#========================================================================================
#Configuration structures

_desc = DeviceDescriptor(DEV_NAME)

core_config = get_core_config(_desc)
device_config = get_dev_config(_desc, core_config)

vector_count = len(_desc.interrupt_map.vectors)
sleep_config = get_sleep_config(_desc.peripherals['SLPCTRL'])
misc_config = get_misc_config(_desc.peripherals['MISC'])

portb_config = get_port_config(_desc.peripherals['PORTB'])
portc_config = get_port_config(_desc.peripherals['PORTC'])
portd_config = get_port_config(_desc.peripherals['PORTD'])
extint_config = get_extint_config(_desc.peripherals['EXTINT'])

timer0_config = get_timer_config(_desc.peripherals['TC0'])
timer1_config = get_timer_config(_desc.peripherals['TC1'])
timer2_config = get_timer_config(_desc.peripherals['TC2'])

adc_config = get_adc_config(_desc.peripherals['ADC'])
acp_config = get_acp_config(_desc.peripherals['ACP'])
vref_bandgap = get_vref_bandgap(_desc.peripherals['VREF'])

usart_config = get_usart_config(_desc.peripherals['USART'])

del _desc

#========================================================================================
#Device class definition

class dev_atmega328(arch_lib.AVR_ArchAVR_Device):

    def __init__(self):
        super(dev_atmega328, self).__init__(device_config)

        intctrl = arch_lib.AVR_ArchAVR_Interrupt(vector_count)
        slpctrl = _corelib.AVR_SleepController(sleep_config)
        misc = arch_lib.AVR_ArchAVR_MiscRegCtrl(misc_config)
        portb = arch_lib.AVR_ArchAVR_Port(portb_config)
        portc = arch_lib.AVR_ArchAVR_Port(portc_config)
        portd = arch_lib.AVR_ArchAVR_Port(portd_config)
        extint = arch_lib.AVR_ArchAVR_ExtInt(extint_config)
        timer0 = arch_lib.AVR_ArchAVR_Timer(0, timer0_config)
        timer1 = arch_lib.AVR_ArchAVR_Timer(1, timer1_config)
        timer2 = arch_lib.AVR_ArchAVR_Timer(2, timer2_config)
        adc = arch_lib.AVR_ArchAVR_ADC(0, adc_config)
        acp = arch_lib.AVR_ArchAVR_ACP(0, acp_config)
        vref = arch_lib.AVR_ArchAVR_VREF(vref_bandgap)
        usart = arch_lib.AVR_ArchAVR_USART(0, usart_config)

        self.attach_peripheral(intctrl)
        self.attach_peripheral(slpctrl)
        self.attach_peripheral(misc)
        self.attach_peripheral(portb)
        self.attach_peripheral(portc)
        self.attach_peripheral(portd)
        self.attach_peripheral(extint)
        self.attach_peripheral(timer0)
        self.attach_peripheral(timer1)
        self.attach_peripheral(timer2)
        self.attach_peripheral(adc)
        self.attach_peripheral(acp)
        self.attach_peripheral(vref)
        self.attach_peripheral(usart)

DEV_CLASS = dev_atmega328

__all__ = ['DEV_NAME', 'DEV_CLASS']
