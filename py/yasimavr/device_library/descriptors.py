# descriptors.py
#
# Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>
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

"""
A descriptor is a Python object storing the information required to build a simulated device model.

There are two ways to load a Device object:
* by name, using one of the builtin device models, by using DeviceDescriptor.create_from_model()
* by file, using DeviceDescriptor.create_from_file()
"""

import weakref
import os
import collections
import re

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
#This can be altered by the user
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


class MemorySegmentDescriptor(collections.namedtuple('MemorySegmentDescriptor', ['start', 'end'])):
    """Named tuple class for a memory segment, representing a continuous addressable range

    :param int start: lowermost address of the range
    :param int end: uppermost address of the range
    """


class MemorySpaceDescriptor:
    """Descriptor class for a memory space, representing an addressing space for the device
    e.g. data space, programe space, eeprom, etc.
    A memory space is composed of one or more 'segments' representing a continuous addressable range.

    :param str name: name of the memory space given by the device descriptor
    :param mem_config: YAML configuration section for the memory space

    :var str name: name given to the memory space
    :var dict[str,MemorySegmentDescriptor] segments: dictionary of segments composing the address space
    :var int memend: uppermost address of the memory space
    """

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

        self.page_size = int(mem_config.get('page_size', 1))


class InterruptMapDescriptor:
    """Descriptor class for an interrupt vector map
    It includes the list of interrupt vectors with their names ordered by index.
    It also includes the sleep mask map. This maps the sleep modes supported by the device
    to the interrupts that can wake up the device.

    Each sleep mask is a list of 8-bits flags where each bit enable or disable a vector.
    Example: with sleep_mask=[0xFF,0x80]: vectors 0 to 7 and 15 are enabled whilst vectors 8 to 14
    and above 15 are disabled.

    :param int_config: YAML configuration section for the interrupt vectors

    :var int vector_size: size in bytes of each vector
    :var list[str] vectors: list of the vector names
    :var dict[name:list[int]] sleep_mask: dict mapping the sleep masks to each sleep mode name
    """

    def __init__(self, int_config):
        self.vector_size = int(int_config['vector_size'])
        self.vectors = list(int_config['vectors'])
        self.sleep_mask =dict(int_config.get('sleep_mask', {}))


class ExtendedBitMask(collections.namedtuple('ExtendedBitMask', ['bit', 'mask'])):

    def as_bitmask(self):
        if self.bit < 8 and self.mask < 0x100:
            return _corelib.bitmask_t(self.bit, self.mask)
        else:
            raise ValueError('Bitmask range error')

    def __repr__(self):
        return 'ExtendedBitMask(%d, 0x%x)' % (self.bit, self.mask)


class RegisterFieldDescriptor:
    """Descriptor class for a field of a I/O register

    :param str field_name: name of the field
    :param field_config: YAML configuration section for the field
    :param int reg_size: size of the register in bits

    Note that variables differ depending on the kind of field

    :var str name: name of the field
    :var str kind: data type of the field: one of 'RAW', 'INT', 'BIT', 'ENUM'
    :var bool readonly: indicates if the field is readonly (true) or writable (false) for the CPU
    :var bool supported: indicates if the field is supported by the simulation model
    :var int pos: (BIT only) bit position
    :var str one: (BIT only) interpretation of the 'one' value (by default: 1)
    :var str zero: (BIT only) interpretation of the 'zero' value (by default: 0)
    :var int LSB: (RAW, INT and ENUM) lowest significant bit position
    :var int MSB: (RAW, INT and ENUM) most significant bit position
    :var dist[int,str] values: (ENUM only) interpretation for the binary values
    :var str unit: (INT only) unit of the values
    """

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
        self.alias = str(field_config.get('alias', ''))

        if self.kind == 'BIT':
            mask = 1 << self.pos
            self._bitmask = ExtendedBitMask(self.pos, mask)
        else:
            mask = ((1 << (self.MSB - self.LSB + 1)) - 1) << self.LSB
            self._bitmask = ExtendedBitMask(self.LSB, mask)


    def bitmask(self):
        return self._bitmask


class RegisterDescriptor:
    """Descriptor class for a I/O register

    :param reg_config: YAML configuration section for the register

    :var str name: name of the register
    :var int address: absolute address of the register (or -1 if not relevant)
    :var int offset: offset of the register from the peripheral base address (or -1 if not relevant)
    :var int size: size in bytes of the register
    :var str kind: kind of the register, one of 'RAW', 'INT' or ''
    :var dict[str,RegisterFieldDescriptor] fields: dict of fields composing the register
    :var bool readonly: indicates if the register is readonly (true) or writable (false) for the CPU
    :var bool supported: indicates if the register is supported by the simulation model
    """

    def __init__(self, reg_config):
        self.name = str(reg_config['name'])

        self.address = self.offset = None
        if 'address' in reg_config:
            self.address = int(reg_config['address'])
        elif 'offset' in reg_config:
            self.offset = int(reg_config['offset'])

        self.size = int(reg_config.get('size', 1))

        self.kind = str(reg_config.get('kind', ''))

        self.fields = {}
        for field_name, field_config in dict(reg_config.get('fields', {})).items():
            f = RegisterFieldDescriptor(field_name, field_config, self.size * 8)
            self.fields[field_name] = f
            if f.alias:
                self.fields[f.alias] = f

        self.readonly = bool(reg_config.get('readonly', False))
        self.supported = bool(reg_config.get('supported', True))

        self.alias = reg_config.get('alias', None)
        if not (self.alias is None or
                isinstance(self.alias, str) or
                (isinstance(self.alias, list) and len(self.alias) == self.size)):
            raise DeviceConfigException('Invalid alias format', reg=self.name)


class ProxyRegisterDescriptor:
    """Descriptor class for a register proxy, used to represent the
    high and low parts of a 16-bits register

    :var RegisterDescriptor reg: full-length register descriptor
    :var int offset: offset of the part from the register address
    """

    def __init__(self, reg, offset):
        self.reg = reg
        self.offset = offset


class PeripheralClassDescriptor:
    """Descriptor class for a peripheral class

    :param per_config: YAML configuration section for the peripheral class

    :var dict[str,RegisterDescriptor] registers: map of the registers owned by the peripheral class
    :var dict config: YAML section storing the settings for configuring the peripheral simulation model
    """

    def __init__(self, per_config):
        self.registers = {}
        for reg_config in list(per_config.get('registers', [])):
            r = RegisterDescriptor(reg_config)
            self.registers[r.name] = r

            if r.size == 2:
                self.registers[r.name + 'L'] = ProxyRegisterDescriptor(r, 0)
                self.registers[r.name + 'H'] = ProxyRegisterDescriptor(r, 1)

            if isinstance(r.alias, str):
                self.registers[r.alias] = ProxyRegisterDescriptor(r, 0)
            elif isinstance(r.alias, list):
                for offset, alias in enumerate(r.alias):
                    self.registers[alias] = ProxyRegisterDescriptor(r, offset)

        self.config = per_config.get('config', {})


#This utility function "consolidate" multiple bitmasks over a
#(possibly multi-byte) register, and merging them in order to
#have only one bitmask per byte maximum.
def _reduce_bitmasks(bms, reg_size):
    mask = 0
    for bm in bms:
        mask |= bm.mask

    if not mask:
        return [(i, _corelib.bitmask_t(0, 0xFF)) for i in range(reg_size)]

    min_bit_shift = min(bm.bit for bm in bms)

    cons_bms = []
    for i in range(reg_size):
        m = (mask >> (i * 8)) & 0xFF
        if m:
            bit_shift = 0 if len(cons_bms) else (min_bit_shift - i * 8)
            cons_bms.append((i, _corelib.bitmask_t(bit_shift, m)))

    return cons_bms


class PeripheralInstanceDescriptor:
    """Descriptor class for the instantiation of a peripheral class.

    For example, a device may have PORTA, PORTB, PORTC, all are instances
    of a PORT peripheral class.

    :var str name: name of the instance
    :var str per_class: class name of the peripheral
    :var str ctl_id: CTLID used for the peripheral model instance
    :var int reg_base: base address of the peripheral (-1 if not relevant)
    :var PeripheralClassDescriptor class_descriptor: descriptor of the class of the peripheral
    :var DeviceDescriptor device: top-level device descriptor owning the peripheral
    :var dict config: YAML section storing the settings for configuring the peripheral model
    """

    def __init__(self, name, loader, f, device):
        self.name = name
        self.per_class = f.get('class', name)
        self.ctl_id = f.get('ctl_id', name[:4])
        self.reg_base = f.get('base', None)
        self.class_descriptor = loader.load_peripheral(self.per_class, f['file'])
        self.device = device
        self.config = f.get('config', {})
        self.register_map = f.get('register_map', {})


    def _resolve_reg_address(self, reg_desc):
        if isinstance(reg_desc, ProxyRegisterDescriptor):
            proxy_addr = self._resolve_reg_address(reg_desc.reg)
            return proxy_addr + reg_desc.offset

        #if the register is mapped by the peripheral instance
        if reg_desc.name in self.register_map:
            return int(self.register_map[reg_desc.name])

        #if the register has a fixed address
        if reg_desc.address is not None:
            return reg_desc.address

        #if the register is defined by an offset from the peripheral base
        if reg_desc.offset is not None and self.reg_base is not None:
            return reg_desc.offset + self.reg_base

        raise DeviceConfigException('Register address cannot be resolved for ' + reg_desc.name)


    def _resolve_regbits(self, reg_name, fields):
        try:
            reg_desc = self.class_descriptor.registers[reg_name]
        except KeyError:
            raise ValueError('Unknown register: ' + reg_name) from None

        reg_addr = self._resolve_reg_address(reg_desc)
        reg_size = reg_desc.size

        #Resolve the remaining fields (given by their names) into shift/masks
        if fields:
            bm_list = []
            for f in fields:
                if isinstance(f, str):
                    f_desc = reg_desc.fields[f]
                    bm = f_desc.bitmask()
                    bm_list.append(bm)
                else:
                    bm_list.append(f)
        else:
            bm_list = [f_desc.bitmask() for f_desc in reg_desc.fields.values()]

        #consolidate the bitmasks
        cons_bm_list = _reduce_bitmasks(bm_list, reg_size)

        #convert the consolidated bitmasks into regbits, with the address
        #incremented for each byte
        rb_list = [_corelib.regbit_t(reg_addr + i, bm) for i, bm in cons_bm_list]

        return rb_list


    def reg_address(self, reg_path, default=None):
        """Utility method that resolves a reg path and returns the address of the register.
        If the register could not be resolved, with a given default value, the default value
        is returned otherwise a ValueError exception is raised.

        :param str reg_path: reg path to a register, must be of format [R] or [P]/[R] (P: peripheral name, R: register name)
        :param any default: if set to any value other than None, sets the default value
        """

        if RegPathSeparator in reg_path:
            return self.device.reg_address(reg_path, default)

        try:
            reg_desc = self.class_descriptor.registers[reg_path]
        except KeyError:
            if default is None:
                raise ValueError('Unknown register: ' + reg_path) from None
            else:
                return default
        else:
            return self._resolve_reg_address(reg_desc)


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
    """Top-level descriptor for a device variant, storing the configuration
    from a YAML configuration file.
    It contains information about pins, interrupts, memory space layout, etc.

    :var str name: name of the device model.
    :var str architecture: one of supported architectures, currently 'AVR' or 'XT'
    :var dict[str,MemorySpaceDescriptor] mem_spaces: dictionary of the memory spaces
    :var dict[str,bool] core_attributes: dictionary of the attribute values to configure the device model
    :var dict[str,int] fuses: default values for the device fuses, usually taken from factory values
    :var dict[str,bool] access_config: dictionary of configuration values for Accessor objects
    :var list[str] pins: list of the pin names
    :var InterruptMapDescriptor interrupt_map: configuration values for interrupts
    :var dict[str,PeripheralInstanceDescriptor] peripherals: dictionary of the peripheral instances
    """

    #Instance cache to speed up the loading of a device several times
    _cache = weakref.WeakValueDictionary()


    @classmethod
    def create_from_model(cls, model):
        """Instantiate a device descriptor from a pre-defined device model configuration.

        :param str model: Model/variant name. the value is case insensitive

        To be valid, the model name must be included in the device list file pointed to by the
        global variable LibraryDatabase.

        Note that device descriptor are cached and a previously instantiated descriptor may be returned.
        """

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
        """Instantiate a device descriptor from an arbitrary configuration file.

        :param str filename: Device configuration file path
        :param list repositories: list of locations for finding peripheral class configuration files
        """

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
        """Utility method that resolves a reg path and returns the address of the register.
        If the register could not be resolved, with a given default value, the default value
        is returned otherwise a ValueError exception is raised.

        :param str reg_path: reg path to a register, must be of format [P]/[R] (P: peripheral name, R: register name)
        :param any default: if set to any value other than None, sets the default value
        """

        elements = _split_reg_path(reg_path)

        if len(elements) != 2:
            raise ValueError('Invalid regpath format: ' + reg_path)

        per_name, reg_name = elements

        try:
            try:
                per_desc = self.peripherals[per_name]
            except KeyError:
                raise ValueError('Unknown peripheral: ' + per_name) from None
            try:
                reg_desc = per_desc.class_descriptor.registers[reg_name]
            except KeyError:
                raise ValueError('Unknown register: ' + reg_name) from None

            return per_desc._resolve_reg_address(reg_desc)

        except ValueError:
            if default is None:
                raise
            else:
                return default


    def _resolve_regbits(self, per_name, reg_name, fields):
        try:
            per_desc = self.peripherals[per_name]
        except KeyError:
            raise ValueError('Unknown peripheral: ' + per_name) from None

        return per_desc._resolve_regbits(reg_name, fields)


#========================================================================================

RegPathSeparator = '/'

def _split_reg_path(reg_path):
    return reg_path.split(RegPathSeparator, 3)


#Utility function that attempts to parse a combination of fields that can be
#  - a bit number
#  - a bit range
#  - a field name
#Returns a list of the parsed fields, containing either bitmask objects for
#the fields that could be immediately parsed, or the name as a string for the others.
def _partial_parse_fields(s):
    #If the field is an integer, it's a bit position
    if isinstance(s, int):
        return [ExtendedBitMask(s, 1 << s)]

    bm_list = []
    for f in s.split('|'):
        #Try to parse as a decimal integer. If so, it is a bit position
        try:
            pos = int(f)
        except ValueError:
            pass
        else:
            bm_list.append(ExtendedBitMask(pos, 1 << pos))
            continue

        #Try a bit range
        range_match = re.fullmatch(r'(?P<lsb>[0-9]{1,2})-(?P<msb>[0-9]{1,2})', f)
        if range_match is not None:
            lsb = int(range_match.group('lsb'))
            msb = int(range_match.group('msb'))
            if lsb > msb: raise ValueError()
            mask = ((1 << (msb - lsb + 1)) - 1) << lsb
            bm_list.append(ExtendedBitMask(lsb, mask))
            continue

        #Append as an unparsed string
        bm_list.append(f)

    return bm_list


#Utility function that parses a reg path to a list of regbits
# path_elements : list of reg path elements as a list
# per: PeripheralInstanceDescriptor object, used for resolving names
# dev: DeviceDescriptor object, used for resolving names
def _parse_regpath(path_elements, per, dev):

    #Utility to try parsing as an integer of any base
    def _parse_num(s):
        try:
            return int(s, 0)
        except ValueError: pass
        return None

    if per is not None:
        dev = per.device

    if len(path_elements) == 1: #format [R]
        parsed_num_addr = _parse_num(path_elements[0])
        if parsed_num_addr is not None:
            return [_corelib.regbit_t(parsed_num_addr)]

        if not per:
            raise ValueError('Peripheral unspecified')

        return per._resolve_regbits(path_elements[0], [])

    elif len(path_elements) == 2: #formats [A, F], [R, F] or [P, R]
        reg_or_per = path_elements[0]

        #Try to parse the first element as if it's numeric. If successful, consider it's a register address
        parsed_num_addr = _parse_num(reg_or_per)
        if parsed_num_addr is not None:
            #Parse the fields
            parsed_fields = _partial_parse_fields(path_elements[1])
            #The format is valid only if all the fields have been already parsed (i.e. they're all numerical)
            if not all(isinstance(f, ExtendedBitMask) for f in parsed_fields):
                raise Exception('All fields must be numeric with a numeric address')

            return [_corelib.regbit_t(parsed_num_addr, f.as_bitmask()) for f in parsed_fields]

        #Try to resolve it as a [R, F] format if a peripheral context is specified
        if per:
            parsed_fields = _partial_parse_fields(path_elements[1])
            try:
                return per._resolve_regbits(reg_or_per, parsed_fields)
            except ValueError: pass

        #If [R, F] failed, try to resolve it as [P, R] format in a device context
        if dev:
            try:
                return dev._resolve_regbits(reg_or_per, path_elements[1], [])
            except ValueError: pass

        raise ValueError('Regpath could not be resolved')

    elif len(path_elements) == 3: #format [P, R, F]
        if not dev:
            raise ValueError('Device unspecified')

        per_name, reg_name, combined_fields = path_elements
        parsed_fields = _partial_parse_fields(combined_fields)
        return dev._resolve_regbits(per_name, reg_name, parsed_fields)

    else:
        raise ValueError('Invalid regpath format')


def convert_to_regbit(arg, per=None, dev=None):
    """Utility method that resolves a reg path and returns a regbit_t object.
    If the register could not be resolved, or if the result spans several bytes,
    a ValueError exception is raised.

    If ``arg`` is an integer, it is interpreted as a register address and a regbit_t object
    representing the whole register is returned.
    If ``arg`` is a list, it is interpreted as the elements of a reg path.

    One of ``per`` or ``dev`` arguments should be specified to resolve reg path with names.
    ``per`` is required to resolve relative reg paths.
    If ``per`` is given, ``dev`` doesn't need to be.

    :param str|int|list arg: reg path to convert
    :param PeripheralInstanceDescriptor per: used to resolve reg paths
    :param DeviceDescriptor dev: used to resolve reg paths
    """

    if arg is None:
        return _corelib.regbit_t()

    if isinstance(arg, int):
        return _corelib.regbit_t(arg)

    if isinstance(arg, (list, tuple)):
        elements = arg
    else:
        elements = _split_reg_path(arg)

    regbit_list = _parse_regpath(elements, per, dev)

    if len(regbit_list) != 1:
        raise ValueError('Was expecting one regbit')

    return regbit_list[0]


def convert_to_regbit_compound(arg, per=None, dev=None):
    """Utility method that resolves a reg path and returns a regbit_compound_t object.
    If the register could not be resolved, a ValueError exception is raised.

    If ``arg`` is a list, it is interpreted as a list of reg path to merge.

    One of ``per`` or ``dev`` arguments should be specified to resolve reg path with names.
    ``per`` is required to resolve relative reg paths.
    If ``per`` is given, ``dev`` doesn't need to be.

    :param str|list[str] arg: reg path or list of reg paths
    :param PeripheralInstanceDescriptor per: used to resolve reg paths
    :param DeviceDescriptor dev: used to resolve reg paths
    """

    if not arg:
        return _corelib.regbit_compound_t()

    if isinstance(arg, (list, tuple)):
        regpath_list = arg
    else:
        regpath_list = [arg]

    regbit_list = []
    for regpath in regpath_list:
        l = _parse_regpath(_split_reg_path(regpath), per, dev)
        regbit_list.extend(l)

        return _corelib.regbit_compound_t(regbit_list)

    else:
        elements = _split_reg_path(arg)
        regbit_list = _parse_regpath(elements, per, dev)
        return _corelib.regbit_compound_t(regbit_list)


def convert_to_bitmask(arg, per=None, dev=None):
    """Utility method that resolves a reg path and returns a bitmask_t object.
    If the register could not be resolved, a ValueError exception is raised.

    If ``arg`` is a list, it is interpreted as a list of reg path to merge.

    One of ``per`` or ``dev`` arguments should be specified to resolve reg path with names.
    ``per`` is required to resolve relative reg paths.
    If ``per`` is given, ``dev`` doesn't need to be.

    :param str|list arg: reg path to a register or field
    :param PeripheralInstanceDescriptor per: used to resolve reg paths
    :param DeviceDescriptor dev: used to resolve reg paths
    """

    if not arg:
        return _corelib.bitmask_t()
    elif isinstance(arg, (list, tuple)):
        regpath_list = arg
    else:
        regpath_list = [arg]

    try:
        flist = _partial_parse_fields('|'.join(regpath_list))
    except ValueError:
        pass
    else:
        if len(flist) == 1 and isinstance(flist[0], ExtendedBitMask):
            return flist[0].as_bitmask()

    rb = convert_to_regbit(arg, per, dev)
    return _corelib.bitmask_t(rb)
