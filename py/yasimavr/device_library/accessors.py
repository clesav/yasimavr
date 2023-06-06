# accessors.py
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
This module defines Accessor classes that allow to read and write
directly to I/O registers with a 'user-friendly' syntax.
It uses the descriptor classes to translate read/write field values
to/from raw bit values and uses a DeviceDebugProbe to access
the registers

These classes are mainly meant for debugging purposes, either
for debugging the peripheral simulation itself such as unit test cases,
or for debugging a firmware by looking at the I/O registers during the
simulation.
'''

from functools import total_ordering

from .descriptors import ProxyRegisterDescriptor
from ..lib import core as _corelib


@total_ordering
class _FieldAccessor:
    '''Generic accessor class for a field of a I/O register.
    Field accessor can be converted and compared to integers
    (it uses the raw bit field value)
    '''

    def __init__(self, reg, field):
        self._reg = reg
        self._field = field

    def __int__(self):
        return self.read_raw()

    def __index__(self):
        return self.read_raw()

    def __lt__(self, other):
        return self.read_raw() < other.__index__()

    def write_raw(self, raw_value):
        shift, bitmask = self._field.shift_mask()
        bitmask <<= shift
        raw_value <<= shift

        rv_in = self._reg.read()
        rv_out = (rv_in & ~bitmask) | (raw_value & bitmask)
        self._reg.write(rv_out)

    def read_raw(self):
        rv = self._reg.read()
        shift, bitmask = self._field.shift_mask()
        return (rv >> shift) & bitmask

    def __str__(self):
        return '%s.%s [%s]' % (self._reg.name, self._field.name, str(self.read()))

    __repr__ = __str__


class BitFieldAccessor(_FieldAccessor):
    '''Accessor class for a field of a I/O register consisting of one bit
    Bit fields can be written with 0, 1, False, True or any value corresponding
    to the defined 'zero' or 'one' parameters in the descriptor.
    '''

    def write(self, value):
        if isinstance(value, int):
            value = bool(value)

        if value in (self._field.one, True):
            self.write_raw(1)
        elif value in (self._field.zero, False):
            self.write_raw(0)
        else:
            raise ValueError('Unknown bit value: ' + str(value))

    def read(self):
        if self.read_raw():
            return self._field.one
        else:
            return self._field.zero

    def __bool__(self):
        return bool(self.read_raw())

    def __eq__(self, other):
        if isinstance(other, int):
            return other == self.read_raw()
        else:
            return other == self.read()


class IntFieldAccessor(_FieldAccessor):
    '''Accessor class for a field of a I/O register consisting of
    an integer value
    '''
    def write(self, value):
        self.write_raw(value.__index__())

    def read(self):
        return self.read_raw()

    def __eq__(self, other):
        return other == self.read_raw()


class RawFieldAccessor(IntFieldAccessor):
    '''Accessor class for a field of a I/O register consisting of
    an raw value. It identical to INT except it's printed in hexadecimal.
    '''

    def __str__(self):
        return '%s.%s [%s]' % (self._reg.name, self._field.name, hex(self.read()))


class EnumFieldAccessor(_FieldAccessor):
    '''Accessor class for a field of a I/O register consisting of
    an enumeration value
    '''

    def write(self, value):
        #If the value is an int, we use it directly
        if isinstance(value, int):
            rv = value
        #If the value is not an int and no enumeration is defined,
        #try to convert to an int. It will raise an exception
        #if it's not possible
        elif self._field.values is None:
            rv = value.__index__()
        #If an enumartion is defined, find it in the values dictionary
        #an use the corresponding index
        else:
            rv = None
            for k, v in self._field.values.items():
                if v == value:
                    rv = k
                    break

            if rv is None:
                raise ValueError('Unknown enum value: ' + str(value))

        self.write_raw(rv)

    def read(self):
        #If no enumeration is defined, return the raw index as value
        rv = self.read_raw()
        if self._field.values is None:
            return rv
        else:
            return self._field.values.get(rv, rv)

    def __eq__(self, other):
        return other == self.read()


@total_ordering
class RegisterAccessor:
    '''Accessor class for a I/O register'''

    def __init__(self, probe, per, addr, reg):
        self._probe = probe
        self._per = per

        if isinstance(reg, ProxyRegisterDescriptor):
            self._reg = reg.reg
            self._addr = addr + reg.offset
        else:
            self._reg = reg
            self._addr = addr

        self._active = True

    @property
    def name(self):
        return '%s.%s' % (self._per.name, self._reg.name)

    @property
    def address(self):
        return self._addr

    def __str__(self):
        if self._reg.kind == 'ARRAY':
            return self.name + ' ' + str(self.read())
        else:
            pattern = '%%s [0x%%0%dx]' % (self._reg.size * 2)
            return pattern % (self.name, self.read())

    __repr__ = __str__

    def __int__(self):
        return self.read()

    def __index__(self):
        return self.read()

    def __eq__(self, other):
        return other == self.read()

    def __lt__(self, other):
        return self.read() < other

    def write(self, value):
        if self._reg.readonly:
            raise ValueError('Cannot write readonly register ' + self.name)

        if not self._reg.supported:
            raise ValueError('Cannot write unsupported register ' + self.name)

        #Easy and most common case first
        if self._reg.size == 1:
            self._probe.write_ioreg(self._addr, value)

        #Array case
        elif self._reg.kind == 'ARRAY':
            for i in range(self._reg.size):
                self._probe.write_ioreg(self._addr + i, value[i])

        #Multi-byte integer.
        #Convert the value to bytes (big-endian) and write the bytes
        #in the order defined by the peripheral byteorder settings
        else:
            lsb_first = self._per._byteorders[0]
            if lsb_first is None:
                raise Exception('Byte order settings are missing')

            byte_indexes = list(range(self._reg.size))
            if not lsb_first:
                byte_indexes.reverse()

            byte_values = value.to_bytes(self._reg.size, byteorder='little')

            for i in byte_indexes:
                self._probe.write_ioreg(self._addr + i, byte_values[i])

    def read(self):
        #Easy and most common case first
        if self._reg.size == 1:
            return self._probe.read_ioreg(self._addr)

        #Array case : no conversion to integer required
        if self._reg.kind == 'ARRAY':
            values = bytes(self._probe.read_ioreg(self._addr + i)
                           for i in range(self._reg.size))
            return values

        #Multi-byte integer.
        #Read the registers in the order defined by the peripheral
        #byteorder settings and convert to integer (big-endian)
        lsb_first = self._per._byteorders[1]
        if lsb_first is None:
            raise Exception('Byte order settings are missing')

        byte_indexes = list(range(self._reg.size))
        if not lsb_first:
            byte_indexes.reverse()

        byte_values = bytearray(self._reg.size)
        for i in byte_indexes:
            byte_values[i] = self._probe.read_ioreg(self._addr + i)

        v = int.from_bytes(byte_values, byteorder='little')
        return v

    def __getattr__(self, key):
        if key.startswith('_'):
            raise AttributeError()
        else:
            field_descriptor = self.__getattribute__('_reg').fields[key]
            return self._get_field_accessor(field_descriptor)

    def __setattr__(self, key, value):
        if getattr(self, '_active', False):
            field_descriptor = self._reg.fields[key]
            accessor = self._get_field_accessor(field_descriptor)
            accessor.write(value)
        else:
            object.__setattr__(self, key, value)

    def _get_field_accessor(self, field_descriptor):
        k = field_descriptor.kind
        if k == 'BIT':
            return BitFieldAccessor(self, field_descriptor)
        elif k == 'INT':
            return IntFieldAccessor(self, field_descriptor)
        elif k == 'RAW':
            return RawFieldAccessor(self, field_descriptor)
        elif k == 'ENUM':
            return EnumFieldAccessor(self, field_descriptor)
        else:
            raise ValueError('Unknown field kind: ' + k)


class PeripheralAccessor:
    '''Accessor class for a peripheral instance'''

    def __init__(self, probe, name, per, byteorders):
        self._probe = probe
        self._name = name
        self._per = per
        self._byteorders = byteorders
        self._active = True

    @property
    def name(self):
        return self._name

    @property
    def class_descriptor(self):
        return self._per.per_class

    @property
    def base(self):
        return self._per.reg_base

    def __getattr__(self, key):
        if key.startswith('_'):
            raise AttributeError()
        else:
            reg_descriptor = self._per.class_descriptor.registers[key]
            reg_addr = self._per.reg_address(key)
            return RegisterAccessor(self._probe, self, reg_addr, reg_descriptor)

    def __setattr__(self, key, value):
        if getattr(self, '_active', False):
            value = value.__index__()
            reg_descriptor = self._per.class_descriptor.registers[key]
            reg_addr = self._per.reg_address(key)
            reg_accessor = RegisterAccessor(self._probe, self, reg_addr, reg_descriptor)
            reg_accessor.write(value)
        else:
            object.__setattr__(self, key, value)


class DeviceAccessor:
    '''Accessor class for a device'''

    def __init__(self, arg, descriptor=None):
        '''Initialisation of a device accessor from either a probe or a device

        If the 1st argument is a probe, it must be already attached to a device.
        If it's a device, the accessor will create a debug probe and attach it.

        descriptor: optional device descriptor object. If not specified, the
                    descriptor is obtained from the _descriptor_ field of the
                    device model instance
        '''

        if isinstance(arg, _corelib.DeviceDebugProbe):
            self._probe = arg
            if not self._probe.attached():
                raise Exception('the probe is not attached to a device')
        elif isinstance(arg, _corelib.Device):
            self._probe = _corelib.DeviceDebugProbe(arg)
        else:
            raise TypeError('First arg must be a device or a attached probe')

        if descriptor is not None:
            self._desc = descriptor
        else:
            self._desc = self._probe.device()._descriptor_

    @property
    def name(self):
        'Name of the device model corresponding to the descriptor used'
        return self._desc.name

    @property
    def aliases(self):
        return tuple(self._desc.aliases)

    @property
    def descriptor(self):
        return self._desc

    def __getattr__(self, key):
        byteorders = (self._desc.access_config.get('lsb_first_on_write', None),
                      self._desc.access_config.get('lsb_first_on_read', None))
        return PeripheralAccessor(self._probe, key, self._desc.peripherals[key], byteorders)
