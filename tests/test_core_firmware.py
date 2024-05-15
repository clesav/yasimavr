# test_core_firmware.py
#
# Copyright 2023 Clement Savergne <csavergne@yahoo.com>
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
import os
import yasimavr.lib.core as corelib
from _test_utils import DictSignalHook


Area = corelib.Firmware.Area


fw_path = os.path.join(os.path.dirname(__file__), 'fw', 'testfw_atmega328.elf')


@pytest.fixture
def firmware():
    return corelib.Firmware.read_elf(fw_path)


def test_datasize(firmware):
    assert firmware.datasize() == 6
    assert firmware.bsssize() == 2


def test_flash(firmware):
    assert firmware.has_memory(Area.Flash)

    blocks = firmware.blocks(Area.Flash)
    assert isinstance(blocks, list)
    assert len(blocks) >= 2 #we should have at least .text, .data

    #Read the .data block and check the content
    stdata_block = [ b for b in blocks if len(b.buf) == 6][0]
    stdata = bytes(stdata_block.buf)
    #The string is "Char" +2 null chars (1 for string termination +1 for word alignement)
    assert stdata == b'Char\0\0'


def test_eeprom(firmware):
    blocks = firmware.blocks(Area.EEPROM)
    assert len(blocks) == 1

    b = blocks[0]
    assert b.base == 0
    assert bytes(b.buf) == b'EEPROM\0'


def test_fuses(firmware):
    blocks = firmware.blocks(Area.Fuses)
    assert len(blocks) == 1

    b = blocks[0]
    assert b.base == 0
    assert bytes(b.buf) == bytes([0xAA, 0xD9, 0xFF])


def test_memory_load(firmware):
    nvm = corelib.NonVolatileMemory(65536)
    firmware.load_memory(Area.Flash, nvm)

    blocks = firmware.blocks(Area.Flash)
    data_block = [ b for b in blocks if len(b.buf) == 6][0]

    m = nvm.block(data_block.base, len(data_block.buf))
    assert bytes(m) == bytes(data_block.buf)
