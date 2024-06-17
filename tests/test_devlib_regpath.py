# test_devlib_regpath.py
#
# Copyright 2024 Clement Savergne <csavergne@yahoo.com>
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
                                                 convert_to_regbit_compound, convert_to_bitmask)
from yasimavr.lib.core import regbit_t


def test_convert_regbit():
    dev = DeviceDescriptor.create_from_model('atmega328')

    rb = convert_to_regbit(None)
    assert not rb.valid()

    rb = convert_to_regbit(0x25)
    assert rb.valid()
    assert rb.addr == 0x25
    assert rb.mask == 0xFF

    with pytest.raises(ValueError):
        rb = convert_to_regbit('SLPCTRL/SMCR/SE')

    rb = convert_to_regbit('SLPCTRL/SMCR/SE', dev=dev)
    assert rb.valid()
    assert rb.addr == 0x33
    assert rb.mask == 0x01

    rb = convert_to_regbit('SMCR/SE', per=dev.peripherals['SLPCTRL'])
    assert rb.valid()
    assert rb.addr == 0x33
    assert rb.mask == 0x01

    rb = convert_to_regbit('SLPCTRL/SMCR', dev=dev)
    assert rb.valid()
    assert rb.addr == 0x33
    assert rb.mask == 0x0F

    rb = convert_to_regbit('SLPCTRL/SMCR/SE|SM', dev=dev)
    assert rb.valid()
    assert rb.addr == 0x33
    assert rb.mask == 0x0F

    rb = convert_to_regbit('0x33/4-5')
    assert rb.valid()
    assert rb.addr == 0x33
    assert rb.mask == 0x30


def test_convert_regbit_compound():
    dev = DeviceDescriptor.create_from_model('atmega4809')

    rbc = convert_to_regbit_compound('USART0/RXDATA/DATA', dev=dev)
    assert len(rbc) == 2
    assert rbc[0].addr == 0x0800
    assert rbc[0].mask == 0xFF
    assert rbc[0].bit == 0
    assert rbc[1].addr == 0x0801
    assert rbc[1].mask == 0x01
    assert rbc[1].bit == 0

    rb = convert_to_regbit('USART0/RXDATA/RXCIF', dev=dev)
    assert rb.addr == 0x0801
    assert rb.mask == 0x80

    rbc = convert_to_regbit_compound('USART0/RXDATA/RXCIF|6-7', dev=dev)
    assert len(rbc) == 2
    assert rbc[0].addr == 0x0800
    assert rbc[0].mask == 0xc0
    assert rbc[0].bit == 6
    assert rbc[1].addr == 0x0801
    assert rbc[1].mask == 0x80
    assert rbc[1].bit == 0


def test_convert_bitmask():
    dev = DeviceDescriptor.create_from_model('atmega328')

    bm = convert_to_bitmask('2-3')
    assert bm.bit == 2
    assert bm.mask == 0x0c

    bm = convert_to_bitmask('4')
    assert bm.bit == 4
    assert bm.mask == 0x10

    bm = convert_to_bitmask('0x33/4-5')
    assert bm.bit == 4
    assert bm.mask == 0x30

    bm = convert_to_bitmask('SLPCTRL/SMCR/SE', dev=dev)
    assert bm.bit == 0
    assert bm.mask == 0x01
