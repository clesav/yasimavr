# __init__.py
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

'''Python package which provides facilities for :
* Device building and configuration
* I/O registry and memory direct access
'''

from .descriptors import *
from .accessors import *
import importlib

dev_class_cache = {}

def load_device(dev_name):
    if dev_name in dev_class_cache:
        return dev_class_cache[dev_name]()

    mod_name = '.builders.device_' + dev_name

    dev_mod = importlib.import_module(mod_name, __package__)

    importlib.invalidate_caches()

    dev_class = dev_mod.DEV_CLASS
    dev_class_cache[dev_name] = dev_class
    return dev_class()

