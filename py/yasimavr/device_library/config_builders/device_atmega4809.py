# device_atmega4809.py
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
This module initialises a ATmega4809
'''

import ..lib.core as _corelib
import ..lib.arch_m0 as _archlib
from ..descriptors import load_descriptor
from .convertors_mega0 import *

DEV_NAME = 'atmega4809'

#========================================================================================
#Configuration structures

_desc = load_descriptor(DEV_NAME)

core_config = get_core_config(_desc)
device_config = get_dev_config(_desc, core_config)
#CPU Interrupt controller
cpuint_config = get_cpuint_config(_desc.peripherals['CPUINT'])
#Sleep controller
sleep_config = get_sleep_config(_desc.peripherals['SLPCTRL'])
#Reset controller
rstctrl_base = _desc.peripherals['RSTCTRL'].reg_base
#Misc registers
misc_config = get_misc_config(_desc.peripherals['MISC'])
#Ports
port_configs = [(s, get_port_config(_desc.peripherals['PORT' + s]))
                for s in ('A', 'B', 'C', 'D', 'E', 'F')]                
#Timer/counters
tca_config = get_tca_config(_desc.peripherals['TCA0'])
tcb_configs = [get_tcb_config(_desc.peripherals['TCB' + str(n)])
               for n in range(4)]
               
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
        self.attach_peripheral(_archlib.AVR_ArchMega0_MiscRegCtrl(misc_config))
        
        #Ports
        for s, cfg in port_configs:
            self.attach_peripheral(_archlib.AVR_ArchMega0_Port(s, cfg))
        
        #Timers/counters
        self.attach_peripheral(_archlib.AVR_ArchMega0_TimerA(tca_config))
        for n, cfg in enumerate(tcb_configs):
            self.attach_peripheral(_archlib.AVR_ArchMega0_TimerB(n, cfg))


DEV_CLASS = dev_atmega4809

__all__ = ['DEV_NAME', 'DEV_CLASS']
