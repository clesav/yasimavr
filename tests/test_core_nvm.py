# test_core_nvm.py
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
import yasimavr.lib.core as corelib


@pytest.fixture
def nvm():
    return corelib.NonVolatileMemory(1024)

data = bytearray([1, 5, 7, 9])


def test_nvm_init(nvm):
    assert nvm.size() == 1024

    #Check that the NVM is initially fully unprogrammed
    for i in range(1024):
        assert not nvm.programmed(i)
        assert nvm[i] == 0xFF


def test_nvm_program(nvm):
    nvm.program(data)
    for i in range(4):
        assert nvm.programmed(i)
        assert nvm[i] == data[i]


def test_nvm_program_with_offset(nvm):
    nvm.program(data, 1)

    assert not nvm.programmed(0)

    for i in range(1, 5):
        assert nvm.programmed(i)
        assert nvm[i] == data[i - 1]


def test_nvm_erase(nvm):
    #Program the pos 1 to 5
    nvm.program(data, 1)
    #Erase the pos 2 to 3
    nvm.erase(2, 2)
    #Check that bytes 1 to 5 except 2 and 3 are programmed
    for i in range(1, 5):
        assert nvm.programmed(i) != (2 <= i <= 3)


def test_nvm_tag_based_write(nvm):
    tag = bytearray([1, 0, 1, 0])
    nvm.spm_write(data, tag, 0)
    #Check that indexes identified by a '1' in tag have been programmed
    #and that indexes identified by a '0' in tag are unchanged
    for i in range(4):
        if tag[i]:
            assert nvm[i] == data[i]
        else:
            assert not nvm.programmed(i)


def test_nvm_tag_based_erase(nvm):
    nvm.program(data)
    tag = bytearray([1, 0, 1, 0])
    nvm.erase(tag, 0)
    #Check that indexes identified by a '1' in tag aren't programmed anymore
    #and that indexes identified by a '0' in tag are unchanged
    for i in range(4):
        if tag[i]:
            assert not nvm.programmed(i)
        else:
            assert nvm[i] == data[i]


def test_nvm_block(nvm):
    nvm.program(data)
    b = nvm.block()

    assert len(b) == 1024
    assert b[:4] == data

    b = nvm.block(2, 2)
    assert b == data[2:4]
