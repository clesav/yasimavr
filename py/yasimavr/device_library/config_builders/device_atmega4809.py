# device_atmega4809.py
#
# Copyright 2022 Clement Savergne <csavergne@yahoo.com>
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

from ...lib import core as _corelib
from ...lib import arch_m0 as _archlib
from ..descriptors import DeviceDescriptor
from .configbuilder import convert_dummy_controller_config
from .convertors_m0 import *

DEV_NAME = 'atmega4809'

#========================================================================================
#Configuration structures

_desc = DeviceDescriptor(DEV_NAME)

core_config = get_core_config(_desc)
device_config = get_dev_config(_desc, core_config)
#CPU Interrupt controller
cpuint_config = get_cpuint_config(_desc.peripherals['CPUINT'])
#Sleep controller
sleep_config = get_sleep_config(_desc.peripherals['SLPCTRL'])
#Reset controller
rstctrl_base = _desc.peripherals['RSTCTRL'].reg_base
#Clock controller
clkctrl_dummy_regs = convert_dummy_controller_config(_desc.peripherals['CLKCTRL'])
#NVM controller
nvmctrl_config = _get_nvmctrl_config(_desc.peripherals['NVMCTRL'])
#Misc registers
misc_config = get_misc_config(_desc.peripherals['MISC'])
#Ports
port_configs = [(s, get_port_config(_desc.peripherals['PORT' + s]))
                for s in ('A', 'B', 'C', 'D', 'E', 'F')]                
#Timer/counters
tca_config = get_tca_config(_desc.peripherals['TCA0'])
tcb_configs = [get_tcb_config(_desc.peripherals['TCB' + str(n)])
               for n in range(4)]
#RTC
rtc_config = get_rtc_config(_desc.peripherals['RTC'])
#ADC
adc_config = get_adc_config(_desc.peripherals['ADC'])
#VREF
vref_base = _desc.peripherals['VREF'].reg_base
#USART
usart_configs = [get_usart_config(_desc.peripherals['USART' + str(n)])
                 for n in range(4)]
#SPI
spi_config = get_spi_config(_desc.peripherals['SPI0'])
#TWI
twi_config = get_twi_config(_desc.peripherals['TWI0'])
#PORTMUX
portmux_dummy_regs = convert_dummy_controller_config(_desc.peripherals['PORTMUX'])
#FUSES
fuses_base = _desc.peripherals['FUSES'].reg_base
#USERROW
userrow_base = _desc.peripherals['USERROW'].reg_base
               
del _desc

#========================================================================================
#Device class definition

class dev_atmega4809(_archlib.AVR_ArchMega0_Device):

    def __init__(self):
        super(dev_atmega4809, self).__init__(device_config)
        
        #Controllers
        self.attach_peripheral(_archlib.AVR_ArchMega0_IntCtrl(cpuint_config))
        self.attach_peripheral(_corelib.AVR_SleepController(sleep_config))
        self.attach_peripheral(_archlib.AVR_ArchMega0_ResetCtrl(rstctrl_base))
        self.attach_peripheral(_corelib.AVR_DummyController(_corelib.IOCTL_CLOCK,
                                                            clkctrl_dummy_regs))
        self.attach_peripheral(_archlib.AVR_ArchMega0_NVM(nvmctrl_config))
        self.attach_peripheral(_archlib.AVR_ArchMega0_MiscRegCtrl(misc_config))
        
        #Ports
        for s, cfg in port_configs:
            self.attach_peripheral(_archlib.AVR_ArchMega0_Port(s, cfg))
        
        #Timers/counters
        self.attach_peripheral(_archlib.AVR_ArchMega0_TimerA(tca_config))
        for n, cfg in enumerate(tcb_configs):
            self.attach_peripheral(_archlib.AVR_ArchMega0_TimerB(n, cfg))
        
        #RTC
        self.attach_peripheral(_archlib.AVR_ArchMega0_RTC(rtc_config))
        
        #ADC, VREF
        self.attach_peripheral(_archlib.AVR_ArchMega0_ADC(adc_config))
        self.attach_peripheral(_archlib.AVR_ArchMega0_VREF(vref_base))
        
        #USARTs
        for n, cfg in enumerate(usart_configs):
            self.attach_peripheral(_archlib.AVR_ArchMega0_USART(n, cfg))
        
        #SPI0
        self.attach_peripheral(_archlib.AVR_ArchMega0_SPI(0, spi_config))
        
        #TWI0
        self.attach_peripheral(_archlib.AVR_ArchMega0_TWI(0, twi_config))
        
        #PORTMUX
        self.attach_peripheral(_corelib.AVR_DummyController(_corelib.IOCTL_PORTMUX,
                                                            portmux_dummy_regs))
        
        #FUSES
        self.attach_peripheral(_archlib.AVR_ArchMega0_Fuses(fuses_base))
        
        #USERROW
        self.attach_peripheral(_archlib.AVR_ArchMega0_USERROW(userrow_base))

DEV_CLASS = dev_atmega4809

__all__ = ['DEV_NAME', 'DEV_CLASS']
