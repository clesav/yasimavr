# device_atmega4808.py
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
This module initialises a ATmega4808
'''

from ._builders_arch_xt import XTDeviceBuilder
from ._dev_mega_0series import dev_mega_0series

def factory_atmega4808(model):
    return XTDeviceBuilder.build_device('atmega4808', dev_mega_0series)

__all__ = ['factory_atmega4808']
