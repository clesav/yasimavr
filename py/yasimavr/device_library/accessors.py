# accessors.py
#
# Copyright 2022-2024 Clement Savergne <csavergne@yahoo.com>
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
This module defines Accessor classes that allow to read and write
directly to I/O registers with a 'user-friendly' syntax.
It uses the descriptor classes to translate read/write field values
to/from raw bit values and uses a DeviceDebugProbe to access
the registers

These classes are mainly meant for debugging purposes, either
for debugging the peripheral simulation itself such as unit test cases,
or for debugging a firmware by looking at the I/O registers during the
simulation.
"""

from functools import total_ordering
import re

from .descriptors import ProxyRegisterDescriptor
from ..lib import core as _corelib


class _ProbeIO:

    def __init__(self, probe):
        self._probe = probe
        self._hold_counter = 0
        self._hold_map = {}

    @property
    def probe(self):
        return self._probe

    def inc_hold(self):
        self._hold_counter += 1

    def dec_hold(self):
        self._hold_counter = max(0, self._hold_counter - 1)
        if not self._hold_counter:
            for addr, value in self._hold_map.items():
                self._probe.write_ioreg(addr, value)
            self._hold_map.clear()

    def read_ioreg(self, addr, as_cpu=True):
        if self._hold_counter and addr in self._hold_map:
            return self._hold_map[addr]
        else:
            return self._probe.read_ioreg(addr, as_cpu)

    def write_ioreg(self, addr, value):
        if not self._hold_counter:
            self._probe.write_ioreg(addr, value)
        else:
            self._hold_map[addr] = value


@total_ordering
class _FieldAccessor:
    """Generic accessor class for a field of a I/O register.
    Field accessor can be converted and compared to integers (it uses the raw bit field value)

    It uses the information stored in a field descriptor
    (:py:class:`RegisterFieldDescriptor <yasimavr.device_library.descriptors.RegisterFieldDescriptor>`)
    to decode/encode the value.
    """

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
        """Write a raw integer value for the field to the I/O register.
        """

        bm = self._field.bitmask()
        rv_in = self._reg.read()
        rv_out = (rv_in & ~bm.mask) | ((raw_value << bm.bit) & bm.mask)
        self._reg.write(rv_out)

    def read_raw(self):
        """Read a raw integer value for the field from the I/O register.
        """

        rv = self._reg.read()
        bm = self._field.bitmask()
        return (rv & bm.mask) >> bm.bit

    def __str__(self):
        return '%s.%s [%s]' % (self._reg.name, self._field.name, str(self.read()))

    def __repr__(self):
        return str(self.read())


class BitFieldAccessor(_FieldAccessor):
    """Accessor class for a field of a I/O register consisting of one bit.

    Bit fields can be written with 0, 1, False, True or any string corresponding
    to the defined 'zero' or 'one' parameters in the descriptor.
    """

    def write(self, value):
        """Write the value for the field to the I/O register.

        :param value integer (raw value), boolean (True or False) or
        the 'one' or 'zero' values defined in the field descriptor.
        """

        if isinstance(value, int):
            value = bool(value)

        if value in (self._field.one, True):
            self.write_raw(1)
        elif value in (self._field.zero, False):
            self.write_raw(0)
        else:
            raise ValueError('Unknown bit value: ' + str(value))

    def read(self):
        """Read the value for the field from the I/O register.

        :return the 'one' or 'zero' values defined in the field descriptor.
        """
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
    """Accessor class for a field of a I/O register consisting of
    an integer value.
    """

    def write(self, value):
        """Write the value for the field to the I/O register.
        """
        self.write_raw(value.__index__())

    def read(self):
        """Read a value for the field from the I/O register.
        """
        return self.read_raw()

    def __eq__(self, other):
        return other == self.read_raw()


class RawFieldAccessor(IntFieldAccessor):
    """Accessor class for a field of a I/O register consisting of
    an raw value. It's identical to IntFieldAccessor except it's printed in hexadecimal.
    """

    def __str__(self):
        return '%s.%s [%s]' % (self._reg.name, self._field.name, hex(self.read()))


class EnumFieldAccessor(_FieldAccessor):
    """Accessor class for a field of a I/O register consisting of
    an enumeration value.
    """

    def write(self, value):
        """Write the value for the field to the I/O register.

        :param value integer (raw value) or a string from one of the enumeration values
        """

        #If the value is an int, we use it directly
        if isinstance(value, int):
            rv = value
        #If the value is not an int and no enumeration is defined,
        #try to convert to an int. It will raise an exception
        #if it's not possible
        elif self._field.values is None:
            rv = value.__index__()
        #If an enumeration is defined, look in the values dictionary
        #and use the corresponding index
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
        """Write the value for the field to the I/O register.

        Return one of the enumeration values if a enum dictionary is specified
        or else the raw integer value.
        """

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
    """Accessor class for a I/O register.

    This class allows to access the whole 8-bits of the register
    or by using each field composing it.

    It supports ordering and comparison to integers.
    """

    def __init__(self, probeIO, per, addr, name, reg):
        self._probeIO = probeIO
        self._per = per
        self._reg_name = name
        self._addr = addr
        self._read_as_cpu = True

        if isinstance(reg, ProxyRegisterDescriptor):
            self._reg = reg.reg
            self._size = 1
        else:
            self._reg = reg
            self._size = reg.size

        self._active = True

    @property
    def name(self):
        """Getter for the register name
        """
        return '%s.%s' % (self._per.name, self._reg_name)

    @property
    def address(self):
        """Getter for the register address
        """
        return self._addr

    @property
    def size(self):
        """Getter for the register size
        """
        return self._size

    @property
    def allocated(self):
        """Returns true is the register is properly allocated in the device model
        """
        return all(self._probeIO.probe.has_ioreg(self._addr + i)
                   for i in range(self._size))

    def set_read_as_cpu(self, enable):
        self._read_as_cpu = enable

    def __str__(self):
        try:
            value = self.read()
        except Exception:
            return self.name + ' [*error*]'

        if self._reg.kind == 'ARRAY':
            return self.name + ' ' + str(value)
        else:
            pattern = '%%s [0x%%0%dx]' % (self._size * 2)
            return pattern % (self.name, value)

    __repr__ = __str__

    def __int__(self):
        return self.read()

    def __index__(self):
        return self.read()

    def __eq__(self, other):
        return other == self.read()

    def __lt__(self, other):
        return self.read() < other

    def _write_byte(self, addr, value):
        if not self._probeIO.probe.has_ioreg(addr):
            raise ValueError('Writing to unallocated register %s [0x%04x]' % (self.name, addr))
        self._probeIO.write_ioreg(addr, value)

    def write(self, value):
        """Write a value to the I/O register

        :param value integer (for INT or RAW kinds) or bytes-like object (for ARRAY kind)

        Raise an exception if the register is read-only or unsupported
        """

        if self._reg.readonly:
            raise ValueError('Cannot write readonly register ' + self.name)

        if not self._reg.supported:
            raise ValueError('Cannot write unsupported register ' + self.name)

        #Easy and most common case first
        if self._size == 1:
            self._write_byte(self._addr, value)

        #Array case
        elif self._reg.kind == 'ARRAY':
            for i in range(self._size):
                self._write_byte(self._addr + i, value[i])

        #Multi-byte integer.
        #Convert the value to bytes (big-endian) and write the bytes
        #in the order defined by the peripheral byteorder settings
        else:
            lsb_first = self._per._byteorders[0]
            if lsb_first is None:
                raise Exception('Byte order settings are missing')

            byte_indexes = list(range(self._size))
            if not lsb_first:
                byte_indexes.reverse()

            byte_values = value.to_bytes(self._size, byteorder='little')

            for i in byte_indexes:
                self._write_byte(self._addr + i, byte_values[i])

    def _read_byte(self, addr):
        if not self._probeIO.probe.has_ioreg(addr):
            raise ValueError('Reading unallocated register %s [0x%04x]' % (self.name, addr))
        return self._probeIO.read_ioreg(addr, self._read_as_cpu)

    def read(self):
        """Read a value from the I/O register

        Return an integer (for INT or RAW kinds) or a bytes object (for ARRAY kind)

        The read always succeeds even for unsupported registers.
        """

        #Easy and most common case first
        if self._size == 1:
            return self._read_byte(self._addr)

        #Array case : no conversion to integer required
        if self._reg.kind == 'ARRAY':
            values = bytes(self._read_byte(self._addr + i)
                           for i in range(self._reg.size))
            return values

        #Multi-byte integer.
        #Read the registers in the order defined by the peripheral
        #byteorder settings and convert to integer (big-endian)
        lsb_first = self._per._byteorders[1]
        if lsb_first is None:
            raise Exception('Byte order settings are missing')

        byte_indexes = list(range(self._size))
        if not lsb_first:
            byte_indexes.reverse()

        byte_values = bytearray(self._size)
        for i in byte_indexes:
            byte_values[i] = self._read_byte(self._addr + i)

        v = int.from_bytes(byte_values, byteorder='little')
        return v

    def __getattr__(self, key):
        if key.startswith('_'):
            raise AttributeError()
        else:
            field_descriptor = self.__getattribute__('_reg').fields[key]
            return self._get_field_accessor(field_descriptor)

    def __setattr__(self, key, value):
        if getattr(self, '_active', False) and not key.startswith('_'):
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

    def __enter__(self):
        self._probeIO.inc_hold()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._probeIO.dec_hold()


class PeripheralAccessor:
    """Accessor class for a peripheral instance
    """

    def __init__(self, probeIO, name, per, byteorders):
        self._probeIO = probeIO
        self._name = name
        self._per = per
        self._byteorders = byteorders
        self._active = True

    @property
    def name(self):
        """Getter for the peripheral name
        """
        return self._name

    @property
    def class_descriptor(self):
        """Getter for the peripheral class descriptor
        """
        return self._per.per_class

    @property
    def base(self):
        """Getter for the peripheral base address (when relevant)
        """
        return self._per.reg_base

    def signal(self):
        """Getter for the peripheral signal (or None if not used)
        """
        ctl_id = _corelib.str_to_id(self._per.ctl_id)
        ok, d = self._probeIO.probe.device().ctlreq(ctl_id, _corelib.CTLREQ_GET_SIGNAL)
        if ok:
            return d.data.as_ptr(_corelib.Signal)
        else:
            return None

    def __getattr__(self, key):
        if key.startswith('_'):
            raise AttributeError()
        else:
            reg_descriptor = self._per.class_descriptor.registers[key]
            reg_addr = self._per.reg_address(key)
            return RegisterAccessor(self._probeIO, self, reg_addr, key, reg_descriptor)

    def __setattr__(self, key, value):
        if getattr(self, '_active', False):
            value = value.__index__()
            reg_descriptor = self._per.class_descriptor.registers[key]
            reg_addr = self._per.reg_address(key)
            reg_accessor = RegisterAccessor(self._probeIO, self, reg_addr, key, reg_descriptor)
            reg_accessor.write(value)
        else:
            object.__setattr__(self, key, value)

    def __enter__(self):
        self._probeIO.inc_hold()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._probeIO.dec_hold()


@total_ordering
class CPURegisterAccessor:
    """Accessor class for a CPU register: R0-R31, X, Y, Z, PC.
    """

    def __init__(self, probe, name, index=-1):
        self._probe = probe
        self._name = name
        self._index = index

    @property
    def name(self):
        """Getter for the register name
        """
        return 'CPU.' + self._name

    @property
    def size(self):
        """Getter for the register size
        """
        return 2 if self._name in ('PC', 'X', 'Y', 'Z') else 1

    def __str__(self):
        try:
            value = self.read()
        except Exception:
            return self.name + ' [*error*]'

        pattern = '%%s [0x%%0%dx]' % (self.size * 2)
        return pattern % (self.name, value)

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
        """Write a value to the I/O register
        :param value integer (8 or 16-bits depending on the register size)
        """

        if self._name == 'PC':
            self._probe.write_pc(value)
        elif self._name == 'X':
            self._probe.write_gpreg(26, value & 0xFF)
            self._probe.write_gpreg(27, (value >> 8) & 0xFF)
        elif self._name == 'Y':
            self._probe.write_gpreg(28, value & 0xFF)
            self._probe.write_gpreg(29, (value >> 8) & 0xFF)
        elif self._name == 'Z':
            self._probe.write_gpreg(30, value & 0xFF)
            self._probe.write_gpreg(31, (value >> 8) & 0xFF)
        else:
            self._probe.write_gpreg(self._index, value & 0xFF)

    def read(self):
        """Read a value from the I/O register
        """

        if self._name == 'PC':
            return self._probe.read_pc()
        elif self._name == 'X':
            return (self._probe.read_gpreg(27) << 8) | self._probe.read_gpreg(26)
        elif self._name == 'Y':
            return (self._probe.read_gpreg(29) << 8) | self._probe.read_gpreg(28)
        elif self._name == 'Z':
            return (self._probe.read_gpreg(31) << 8) | self._probe.read_gpreg(30)
        else:
            self._probe.read_gpreg(self._index)


class CPUAccessor(PeripheralAccessor):
    """Accessor class for the core, giving access to the CPU registers (Rxx, PC) along with
    access to those located in the I/O space (SREG, SP)
    """

    def _match_GPREG(self, key):
        if re.fullmatch('R[0-9]{1,2}', key):
            i = int(key[1:])
            if 0 <= i < 32:
                return i
        return None

    def __getattr__(self, key):
        if key in ('PC', 'X', 'Y', 'Z'):
            return CPURegisterAccessor(self._probeIO.probe, key)

        gpreg_index = self._match_GPREG(key)
        if gpreg_index is not None:
            return CPURegisterAccessor(self._probeIO.probe, key, gpreg_index)

        return super().__getattr__(key)

    def __setattr__(self, key, value):
        if not getattr(self, '_active', False):
            object.__setattr__(self, key, value)
            return

        if key in ('PC', 'X', 'Y', 'Z'):
            reg_accessor = CPURegisterAccessor(self._probeIO.probe, key)
            reg_accessor.write(value)
            return

        gpreg_index = self._match_GPREG(key)
        if gpreg_index is not None:
            reg_accessor = CPURegisterAccessor(self._probeIO.probe, key, gpreg_index)
            reg_accessor.write(value)
            return

        super().__setattr__(key, value)


class DeviceAccessor:
    """Accessor class for a device.

    It is initialised from either a probe or a device.

    If the 1st argument is a probe, it must be already attached to a device.
    If it's a device, the accessor will create a debug probe and attach it.

    :param DeviceDebugProbe|Device arg:
    :param DeviceDescriptor descriptor: optional device descriptor object. If not specified, the
        descriptor is obtained from the _descriptor_ field of the device model instance.

    :var dict pins: dictionary of the device model pins
    """

    def __init__(self, arg, descriptor=None):
        if isinstance(arg, _corelib.DeviceDebugProbe):
            if not arg.attached():
                raise Exception('the probe is not attached to a device')
            self._probe = arg
            dev_model = arg.device()
        elif isinstance(arg, _corelib.Device):
            self._probe = _corelib.DeviceDebugProbe(arg)
            dev_model = arg
        else:
            raise TypeError('First arg must be a device or a attached probe')

        self._probeIO = _ProbeIO(self._probe)

        if descriptor is not None:
            self._desc = descriptor
        else:
            self._desc = dev_model._descriptor_

        #Convenience dictionary for accessing the pins of the device model
        self.pins = {name : dev_model.find_pin(name) for name in self._desc.pins}

        #Convenience dictionary for accessing the non-volatile memories
        self.nvms = {}
        for n, v in dev_model._NVMs_.items():
            req = _corelib.ctlreq_data_t()
            req.index = v
            ok, req = dev_model.ctlreq(_corelib.IOCTL_CORE, _corelib.CTLREQ_CORE_NVM, req)
            if ok:
                self.nvms[n] = req.data.value(_corelib.NonVolatileMemory)

    @property
    def name(self):
        """Name of the device model corresponding to the descriptor used
        """
        return self._desc.name

    @property
    def aliases(self):
        """Aliases of the device model corresponding to the descriptor used
        """
        return tuple(self._desc.aliases)

    @property
    def descriptor(self):
        """Getter for the device descriptor
        """
        return self._desc

    def __getattr__(self, key):
        byteorders = (self._desc.access_config.get('lsb_first_on_write', None),
                      self._desc.access_config.get('lsb_first_on_read', None))

        try:
            per_descriptor = self._desc.peripherals[key]
        except KeyError:
            raise NameError(key + ' : unknown peripheral') from None

        if key == 'CPU':
            return CPUAccessor(self._probeIO, key, per_descriptor, byteorders)

        if self._probe.device().find_peripheral(per_descriptor.ctl_id) is None:
            raise AttributeError(key + ' : peripheral not attached')

        return PeripheralAccessor(self._probeIO, key, per_descriptor, byteorders)
