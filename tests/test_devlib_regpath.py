# test_devlib_regpath.py
#
# Copyright 2024-2026 Clement Savergne <csavergne@yahoo.com>
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


import pytest
from yasimavr.device_library.descriptors import (DeviceDescriptor, convert_to_regbit,
                                                 convert_to_regbit_compound, convert_to_bitspec,
                                                 convert_to_regmask)
from yasimavr.lib.core import regbit_t, regmask_t, reg_addr_t, bitmask_t


def test_convert_regbit():
    dev = DeviceDescriptor.create_from_model('atmega328')

    rb = convert_to_regbit(None)
    assert not rb.valid()

    res = convert_to_regbit(0x25)
    assert isinstance(res, regbit_t)
    assert res.valid()
    assert res.addr == 0x25
    assert bitmask_t(res).mask == 0xFF

    #Should throw an exception because neither 'per' not 'dev' are specified
    with pytest.raises(ValueError):
        rb = convert_to_regbit('SLPCTRL/SMCR/SE')

    rb = convert_to_regbit('SLPCTRL/SMCR/SE', dev=dev)
    assert rb.valid()
    assert rb.addr == 0x33
    assert bitmask_t(rb).mask == 0x01

    rb = convert_to_regbit('SMCR/SE', per=dev.peripherals['SLPCTRL'])
    assert rb.valid()
    assert rb.addr == 0x33
    assert bitmask_t(rb).mask == 0x01

    #Should throw an exception because multiple fields are returned
    with pytest.raises(ValueError):
        rb = convert_to_regbit('SLPCTRL/SMCR', dev=dev)

    #Should throw an exception because multiple fields are returned
    with pytest.raises(ValueError):
        rb = convert_to_regbit('SLPCTRL/SMCR/SE|SM', dev=dev)

    #Should no throw an exception because merge_bits is True
    rb = convert_to_regbit('SLPCTRL/SMCR/SE|SM', dev=dev, merge_bits=True)
    assert rb.addr == 0x33
    assert bitmask_t(rb).mask == 0x0F

    #Should throw an exception because of a byte size more than 1
    with pytest.raises(ValueError):
        rb = convert_to_regbit('NVMCTRL/EEAR')

    rb = convert_to_regbit('0x33/4-5')
    assert rb.valid()
    assert rb.addr == 0x33
    assert bitmask_t(rb).mask == 0x30


def test_convert_regbit_compound():
    dev = DeviceDescriptor.create_from_model('atmega4809')

    rbc = convert_to_regbit_compound('USART0/RXDATA/DATA', dev=dev)
    assert len(rbc) == 2
    assert rbc[0].addr == 0x0800
    assert bitmask_t(rbc[0]).mask == 0xFF
    assert rbc[1].addr == 0x0801
    assert bitmask_t(rbc[1]).mask == 0x01

    rb = convert_to_regbit('USART0/RXDATA/RXCIF', dev=dev)
    assert rb.addr == 0x0801
    assert bitmask_t(rb).mask == 0x80

    rbc = convert_to_regbit_compound('USART0/RXDATA/RXCIF|6-7', dev=dev)
    assert len(rbc) == 2
    assert rbc[0].addr == 0x0800
    assert bitmask_t(rbc[0]).mask == 0xc0
    assert rbc[1].addr == 0x0801
    assert bitmask_t(rbc[1]).mask == 0x80


def test_convert_regmask():
    dev = DeviceDescriptor.create_from_model('atmega4809')

    #Should throw an exception because of a byte size more than 1
    with pytest.raises(ValueError):
        rm = convert_to_regmask('USART0/RXDATA', dev=dev)

    rm = convert_to_regmask('USART0/RXDATA/RXCIF|FERR', dev=dev)
    assert isinstance(rm, regmask_t)
    assert rm.addr == 0x0801
    assert rm.mask.mask == 0x84


def test_convert_bitspec():
    dev = DeviceDescriptor.create_from_model('atmega328')

    bs = convert_to_bitspec('2-3')
    assert bs.lsb() == 2
    assert bs.msb() == 3

    bs = convert_to_bitspec('4')
    assert bs.lsb() == 4
    assert bitmask_t(bs).mask == 0x10

    bs = convert_to_bitspec('0x33/4-5')
    assert bs.lsb() == 4
    assert bs.msb() == 5
    assert bitmask_t(bs).mask == 0x30

    bs = convert_to_bitspec('SLPCTRL/SMCR/SE', dev=dev)
    assert bs.lsb() == 0
    assert bs.msb()== 0
