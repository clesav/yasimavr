# __init__.py
#
# Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>
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

from ._device_db import device_database
from .descriptors import DeviceDescriptor
from .accessors import DeviceAccessor
import importlib
import os.path


_factory_cache = {}

def load_device(dev_name, verbose=False):
    base_dev_name = os.path.basename(dev_name).lower()

    cache_entry = _factory_cache.get(base_dev_name, None)
    if cache_entry is not None:
        if verbose:
            print('Using device factory from cache')
        dev_factory, dev_fullname = cache_entry
        return dev_factory(dev_fullname)

    from .builders import _base
    _base.VERBOSE = verbose

    dev_factory_modname, full_dev_name = device_database.find_device_entry(base_dev_name)

    mod_name = '.builders.' + dev_factory_modname
    if verbose:
        print('Loading device factory module', mod_name)

    dev_mod = importlib.import_module(mod_name, __package__)
    importlib.invalidate_caches()

    dev_factory = getattr(dev_mod, 'device_factory')
    _factory_cache[base_dev_name] = (dev_factory, full_dev_name)
    return dev_factory(full_dev_name)


def load_device_from_config(dev_descriptor, dev_class=None, verbose=False):
    from .builders import _base
    _base.VERBOSE = verbose

    arch = dev_descriptor.architecture
    if arch == 'AVR':
        from .builders._builders_arch_avr import AVR_DeviceBuilder, AVR_BaseDevice

        if dev_class is None:
            dev_class = AVR_BaseDevice
            do_peripheral_build = True
        elif not issubclass(dev_class, AVR_BaseDevice):
            raise TypeError('the device class must be a AVR_BaseDevice subclass')
        else:
            do_peripheral_build = False

        dev = AVR_DeviceBuilder.build_device(dev_descriptor, dev_class)

    elif arch == 'XT':
        from .builders._builders_arch_xt import XT_DeviceBuilder, XT_BaseDevice

        if dev_class is None:
            dev_class = XT_BaseDevice
            do_peripheral_build = True
        elif not issubclass(dev_class, XT_BaseDevice):
            raise TypeError('the device class must be a XT_BaseDevice subclass')
        else:
            do_peripheral_build = False

        dev = XT_DeviceBuilder.build_device(dev_descriptor, dev_class)

    else:
        raise ValueError('Architecture unknown: ' + arch)

    if do_peripheral_build:
        per_name_list = dev_descriptor.peripherals.keys()
        dev._builder_.build_peripherals(dev, per_name_list)

    return dev


def model_list():
    return device_database.model_list()


__all__ = ['load_device',
           'load_device_from_config',
           'model_list',
           'DeviceDescriptor',
           'DeviceAccessor']
