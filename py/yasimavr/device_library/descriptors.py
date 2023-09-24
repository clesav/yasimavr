# descriptors.py
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
This module defines Descriptor classes that contain the variant
configuration decoded from YAML configuration files
'''

import weakref
import os
import collections

from ..lib import core as _corelib


from yaml import load as _yaml_load
try:
    from yaml import CLoader as _YAMLLoader
except ImportError:
    from yaml import SafeLoader as _YAMLLoader


Architectures = ['AVR', 'XT']

LibraryRepository = os.path.join(os.path.dirname(__file__), 'configs')
LibraryModelDatabase = os.path.join(LibraryRepository, 'devices.yml')

#List of path which are searched for YAML configuration files
#This can be altered by the used
ConfigRepositories = [
    LibraryRepository
]


def _find_config_file(fn, repositories):
    for r in repositories:
        p = os.path.join(r, fn)
        if os.path.isfile(p):
            return p
    return None


def load_config_file(fn):
    with open(fn) as f:
        return _yaml_load(f, _YAMLLoader)


class DeviceConfigException(Exception):
    pass


#Internal utility that returns a absolute register address based
#on its offset and the peripheral base address if it has one.
def _get_reg_address(reg_descriptor, base):

    if isinstance(reg_descriptor, ProxyRegisterDescriptor):
        return _get_reg_address(reg_descriptor.reg, base) + reg_descriptor.offset
    else:
        if reg_descriptor.address >= 0:
            return reg_descriptor.address
        elif base >= 0 and reg_descriptor.offset >= 0:
            return base + reg_descriptor.offset
        else:
            raise DeviceConfigException('Register address cannot be resolved for ' + reg_descriptor.name)


MemorySegmentDescriptor = collections.namedtuple('_MemorySegmentDescriptor', ['start', 'end'])


class MemorySpaceDescriptor:
    '''Descriptor class for a memory space'''

    def __init__(self, name, mem_config):
        self.name = name
        self.segments = {}
        memend = 0

        mem_config_lc = {k.lower(): v for k, v in mem_config.items()}
        for segmarker in mem_config_lc:
            if segmarker.endswith('end') and segmarker != 'memend':
                segend = int(mem_config_lc[segmarker])

                segstartmarker = segmarker.replace('end', 'start')
                segstart = int(mem_config_lc.get(segstartmarker, 0))

                segname = segmarker.replace('end', '').lower()
                self.segments[segname] = MemorySegmentDescriptor(segstart, segend)

                memend = max(memend, segend)

        self.memend = int(mem_config.get('memend', memend))


class InterruptMapDescriptor:
    '''Descriptor class for a interrupt vector map'''

    def __init__(self, int_config):
        self.vector_size = int(int_config['vector_size'])
        self.vectors = list(int_config['vectors'])
        self.sleep_mask =dict(int_config.get('sleep_mask', {}))


class RegisterFieldDescriptor:
    '''Descriptor class for a field of a I/O register'''

    def __init__(self, field_name, field_config, reg_size):
        self.name = field_name
        self.kind = str(field_config.get('kind', 'RAW'))

        if self.kind == 'BIT':
            self.pos = int(field_config['pos'])
            self.one = field_config.get('one', 1)
            self.zero = field_config.get('zero', 0)

        elif self.kind == 'ENUM':
            self.LSB = int(field_config.get('LSB', 0))
            self.MSB = int(field_config.get('MSB', reg_size - 1))

            self.values = {}

            fvalues = field_config.get('values', None)
            if isinstance(fvalues, list):
                self.values = {i: v for i, v in enumerate(fvalues)}
            elif isinstance(fvalues, dict):
                self.values = dict(fvalues)
            else:
                self.values = None

            if self.values is not None:
                fvalues = field_config.get('values2', {})
                self.values.update(fvalues)

        elif self.kind == 'INT':
            self.LSB = int(field_config.get('LSB', 0))
            self.MSB = int(field_config.get('MSB', reg_size - 1))
            self.unit = str(field_config.get('unit', ''))

        elif self.kind == 'RAW':
            self.LSB = int(field_config.get('LSB', 0))
            self.MSB = int(field_config.get('MSB', reg_size - 1))

        self.readonly = bool(field_config.get('readonly', False))
        self.supported = bool(field_config.get('supported', True))

    def shift_mask(self):
        if self.kind == 'BIT':
            return self.pos, 1
        else:
            mask = (1 << (self.MSB - self.LSB + 1)) - 1
            return self.LSB, mask


class RegisterDescriptor:
    '''Descriptor class for a I/O register'''

    def __init__(self, reg_config):
        self.name = str(reg_config['name'])

        if 'address' in reg_config:
            self.address = int(reg_config['address'])
        elif 'offset' in reg_config:
            self.address = -1
            self.offset = int(reg_config['offset'])
        else:
            raise DeviceConfigException('No address for register ' + self.name)

        self.size = int(reg_config.get('size', 1))

        self.kind = str(reg_config.get('kind', ''))

        self.fields = {}
        for field_name, field_config in dict(reg_config.get('fields', {})).items():
            self.fields[field_name] = RegisterFieldDescriptor(field_name, field_config, self.size * 8)

        self.readonly = bool(reg_config.get('readonly', False))
        self.supported = bool(reg_config.get('supported', True))

    def bitmask(self, field_names=None):
        if field_names is None:
            fields = self.fields.values()
        else:
            fields = [f for n, f in self.fields.items() if n in field_names]

        bitmasks = []
        for by in range(self.size):
            bit = 7
            mask = 0
            for f in fields:
                b, m = f.shift_mask()
                b -= 8 * by
                m >>= 8 * by
                if m & 0xFF:
                    b = max(0, b)
                    m &= 0xFF
                    bit = min(bit, b)
                    mask |= m << b

            bitmasks.append(_corelib.bitmask_t(bit, mask))

        return bitmasks[0] if self.size == 1 else bitmasks


class ProxyRegisterDescriptor:
    '''Descriptor class for a register proxy, used to represent the
    high and low parts of a 16-bits register'''

    def __init__(self, r, offset):
        self.reg = r
        self.offset = offset


class PeripheralClassDescriptor:
    '''Descriptor class for a peripheral type'''

    def __init__(self, per_config):
        self.registers = {}
        for reg_config in list(per_config.get('registers', [])):
            r = RegisterDescriptor(reg_config)
            self.registers[r.name] = r

            if r.size == 2:
                self.registers[r.name + 'L'] = ProxyRegisterDescriptor(r, 0)
                self.registers[r.name + 'H'] = ProxyRegisterDescriptor(r, 1)

        self.config = per_config.get('config', {})


class PeripheralInstanceDescriptor:
    '''Descriptor class for the instantiation of a peripheral type'''

    def __init__(self, name, loader, f, device):
        self.name = name
        self.per_class = f.get('class', name)
        self.ctl_id = f.get('ctl_id', name[:4])
        self.reg_base = f.get('base', -1)
        self.class_descriptor = loader.load_peripheral(self.per_class, f['file'])
        self.device = device
        self.config = f.get('config', {})

    def reg_descriptor(self, reg_name):
        if '.' in reg_name:
            per_name, reg_name = reg_name.split('.', 1)
            per = self.device.peripherals[per_name]
        else:
            per = self

        return per.class_descriptor.registers[reg_name]

    def reg_address(self, reg_name, default=None):
        return self.device._reg_address(reg_name, self, default)


#Utility class that manages caches of peripheral configurations and
#loaded YAML configuration files
#This is only used at configuration loading time and discarded once
#the configuration loading is complete
class _DeviceDescriptorLoader:

    def __init__(self, yml_cfg, repositories):
        self.cfg = yml_cfg
        self.repositories = repositories
        self._yml_cache = {}
        self._per_cache = {}

    def load_peripheral(self, per_name, per_filepath):
        if per_name in self._per_cache:
            return self._per_cache[per_name]

        if per_filepath in self._yml_cache:
            per_yml_doc = self._yml_cache[per_filepath]
        else:
            per_path = _find_config_file(per_filepath, self.repositories)
            if per_path is None:
                raise DeviceConfigException('Config file not found: ' + per_filepath)

            per_yml_doc = load_config_file(per_path)
            self._yml_cache[per_filepath] = per_yml_doc

        per_config = per_yml_doc[per_name]
        per_descriptor = PeripheralClassDescriptor(per_config)
        self._per_cache[per_name] = per_descriptor
        return per_descriptor


class DeviceDescriptor:
    '''Top-level descriptor for a device variant, storing the configuration
    from a YAML configuration file.
    '''

    #Instance cache to speed up the loading of a device several times
    _cache = weakref.WeakValueDictionary()


    @classmethod
    def create_from_model(cls, model):
        lower_model = model.lower()
        if lower_model in cls._cache:
            return cls._cache[lower_model]

        fn = _find_config_file(lower_model + '.yml', ConfigRepositories)
        if fn is None:
            raise DeviceConfigException('No configuration found for variant ' + model)

        try:
            yml_cfg = load_config_file(fn)
        except Exception as exc:
            msg = 'Error reading the configuration file for ' + model
            raise DeviceConfigException(msg) from exc

        desc = cls()
        desc._load_config(yml_cfg, ConfigRepositories)
        cls._cache[lower_model] = desc
        return desc


    @classmethod
    def create_from_file(cls, filename, repositories=()):
        if repositories:
            r = repositories + ConfigRepositories
        else:
            r = [os.path.dirname(filename)] + ConfigRepositories

        try:
            yml_cfg = load_config_file(filename)
        except Exception as exc:
            msg = 'Error reading the configuration file'
            raise DeviceConfigException(msg) from exc

        desc = cls()
        desc._load_config(yml_cfg, r)
        return desc


    def _load_config(self, yml_cfg, repositories):

        self.name = str(yml_cfg['name'])

        if 'aliasof' in yml_cfg:
            alias = str(yml_cfg['aliasof']).lower()
            yml_cfg = self._read_config(alias, repositories)

        dev_loader = _DeviceDescriptorLoader(yml_cfg, repositories)

        self.architecture = str(yml_cfg['architecture'])
        if self.architecture not in Architectures:
            raise DeviceConfigException('Unsupported architecture: ' + self.architecture)

        self.mem_spaces = {}
        for name, f in dict(yml_cfg['memory']).items():
            self.mem_spaces[name] = MemorySpaceDescriptor(name, f)

        self.core_attributes = dict(yml_cfg['core'])

        self.fuses = dict(yml_cfg.get('fuses', {}))

        self.access_config = dict(yml_cfg.get('access', {}))

        self.pins = list(yml_cfg['pins'])

        self.interrupt_map = InterruptMapDescriptor(dict(yml_cfg['interrupts']))

        self.peripherals = {}
        for per_name, f in dict(yml_cfg['peripherals']).items():
            self.peripherals[per_name] = PeripheralInstanceDescriptor(per_name, dev_loader, f, self)


    def reg_address(self, reg_path, default=None):
        return self._reg_address(reg_path, None, default)


    def _reg_address(self, reg_path, per_from, default):
        if '.' in reg_path:
            per_name, reg_name = reg_path.split('.', 1)
            try:
                per = self.peripherals[per_name]
            except KeyError:
                if default is None:
                    raise DeviceConfigException('Unknown peripheral') from None
                else:
                    return default

        elif isinstance(per_from, PeripheralInstanceDescriptor):
            per = per_from
            reg_name = reg_path

        else:
            raise DeviceConfigException('Invalid register name')

        try:
            reg_descriptor = per.class_descriptor.registers[reg_name]
        except KeyError:
            if default is None:
                raise DeviceConfigException('Unknown register') from None
            else:
                return default
        else:
            return _get_reg_address(reg_descriptor, per.reg_base)
