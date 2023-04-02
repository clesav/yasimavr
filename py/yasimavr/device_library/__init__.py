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

from .descriptors import DeviceDescriptor, LibraryModelDatabase, load_config_file
from .accessors import DeviceAccessor
import importlib
import os.path

_factory_cache = {}

def load_device(dev_name, verbose=False):
    low_dev_name = dev_name.lower()

    if low_dev_name in _factory_cache:
        if verbose:
            print('Using device factory from cache')
        return _factory_cache[low_dev_name](low_dev_name)

    from .builders import _base
    _base.VERBOSE = verbose

    device_db = load_config_file(LibraryModelDatabase)
    for f, dev_list in device_db.items():
        if low_dev_name in dev_list:
            dev_factory = f
            break
    else:
        raise Exception('No model found for ' + dev_name)

    mod_name = '.builders.' + dev_factory
    if verbose:
        print('Loading device factory module', mod_name)

    dev_mod = importlib.import_module(mod_name, __package__)
    importlib.invalidate_caches()

    factory = getattr(dev_mod, 'device_factory')
    _factory_cache[low_dev_name] = factory
    return factory(low_dev_name)


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
    device_db = load_config_file(LibraryModelDatabase)
    models = []
    for dev_list in device_db.values():
        models.extend(dev_list)

    return models


__all__ = ['load_device',
           'load_device_from_config',
           'model_list',
           'DeviceDescriptor',
           'DeviceAccessor']
