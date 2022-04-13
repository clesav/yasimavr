# accessors.py
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
This module defines Accessor classes that allow to read and write
directly to I/O registers with a 'user-friendly' syntax.
It uses the descriptor classes to translate read/write field values
to/from raw bit values and uses a AVR_DeviceDebugProbe to access
the registers  

These classes are mainly meant for debugging purposes, either
for debugging the peripheral simulation itself such as unit test cases,
or for debugging a firmware by looking at the I/O registers during the
simulation. 
'''

from functools import total_ordering

from .descriptors import DeviceDescriptor


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
        self._addr = addr
        self._reg = reg
        self._active = True
    
    @property
    def name(self):
        return '%s.%s' % (self._per.name, self._reg.name)
    
    @property
    def address(self):
        return self._addr
    
    def __str__(self):
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
        
        if self._reg.size == 1:            
            self._probe.write_ioreg(self._addr, value)
        else:
            assert self._reg.size == 2
            
            big_endian = self._per._endians[0]
            if big_endian is None:
                raise Exception('Endian settings are missing')
            elif big_endian:
                self._probe.write_ioreg(self._addr, value & 0xFF)
                self._probe.write_ioreg(self._addr + 1, (value >> 8) & 0xFF)
            else:
                self._probe.write_ioreg(self._addr + 1, (value >> 8) & 0xFF)
                self._probe.write_ioreg(self._addr, value & 0xFF)
    
    def read(self):
        if self._reg.size == 1:
            return self._probe.read_ioreg(self._addr)
        else:
            assert self._reg.size == 2
            
            big_endian = self._per._endians[1]
            if big_endian is None:
                raise Exception('Endian settings are missing')
            elif big_endian:
                vl = self._probe.read_ioreg(self._addr)
                vh = self._probe.read_ioreg(self._addr + 1)
                return (vh << 8) + vl
            else:
                vh = self._probe.read_ioreg(self._addr + 1)
                vl = self._probe.read_ioreg(self._addr)
                return (vh << 8) + vl
    
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
    
    def __init__(self, probe, name, per, endians):
        self._probe = probe
        self._name = name
        self._per = per
        self._endians = endians
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


class AVR_DeviceAccessor:
    '''Accessor class for a device'''
    
    def __init__(self, probe):
        '''Initialisation of a device accessor
        probe : device probe, must be already attached to a device
        '''
        self._probe = probe
        self._dev = DeviceDescriptor(probe.device().config().name)

    @property
    def name(self):
        return self._dev.name
    
    @property
    def aliases(self):
        return tuple(self._dev.aliases)
    
    @property
    def descriptor(self):
        return self._dev
    
    def __getattr__(self, key):
        endians = (self._dev.access_config.get('big_endian_on_write', None),
                   self._dev.access_config.get('big_endian_on_read', None))
        return PeripheralAccessor(self._probe, key, self._dev.peripherals[key], endians)
