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

from ._builders_arch_avr import AVR_DeviceBuilder
from ._dev_megaxx8 import dev_megaxx8

def factory_atmega328(model):
    return AVR_DeviceBuilder.build_device('atmega328', dev_megaxx8)

__all__ = ['factory_atmega328']
