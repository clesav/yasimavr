# test_xt_sections.py
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
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_xt as archlib
from _test_bench_xt import BenchXT, TESTFW_M4809
from yasimavr.device_library import load_device_from_config, DeviceDescriptor
from _test_utils import DictSignalHook


@pytest.fixture
def bench():
    bench = BenchXT('atmega4809', TESTFW_M4809)

    _, req = bench.dev_model.ctlreq(corelib.IOCTL_CORE, corelib.CTLREQ_CORE_SECTIONS)
    bench.sections = req.data.value(corelib.MemorySectionManager)

    return bench


Sections = archlib.ArchXT_Device.FlashSection

def _sections_sizes(s):
    return [ s.section_size(i) for i in Sections ]

#
def test_xt_sections(bench):
    '''Check the section limit settings by the fuses
    '''

    fuse_nvm = bench.dev.nvms['Fuses']

    #Setting BOOTEND to 4
    fuse_nvm.dbg_write(100, 0x08)
    #Check that the update is only taken into account on a reset
    assert bench.dev.FUSES.BOOTEND == 0
    assert _sections_sizes(bench.sections) == [192, 0, 0]
    bench.dev_model.reset()
    assert bench.dev.FUSES.BOOTEND == 100
    assert _sections_sizes(bench.sections) == [100, 92, 0]

    #Setting APPEND to 150
    fuse_nvm.dbg_write(150, 0x07)
    assert bench.dev.FUSES.APPEND == 0
    bench.dev_model.reset()
    assert bench.dev.FUSES.APPEND == 150
    assert _sections_sizes(bench.sections) == [100, 50, 42]

    #Setting APPEND to 50 => the section disappears
    fuse_nvm.dbg_write(50, 0x07)
    bench.dev_model.reset()
    assert _sections_sizes(bench.sections) == [100, 0, 92]


def test_xt_sections_write_protections(bench):
    '''Check the write protections depending on which
    section is fetched
    '''

    fuse_nvm = bench.dev.nvms['Fuses']
    fuse_nvm.dbg_write(80, 0x08)
    fuse_nvm.dbg_write(120, 0x07)
    bench.dev_model.reset()

    assert bench.sections.fetch_address(50*256)
    assert bench.sections.can_write(100*256)
    assert bench.sections.can_write(150*256)

    assert bench.sections.fetch_address(100*256)
    assert not bench.sections.can_write(50*256)
    assert bench.sections.can_write(150*256)

    assert bench.sections.fetch_address(150*256)
    assert not bench.sections.can_write(50*256)
    assert not bench.sections.can_write(100*256)


def test_xt_sections_signal(bench):
    '''Check the signalling when entering and leaving sections
    '''

    SigId = corelib.MemorySectionManager.SignalId

    hook = DictSignalHook(bench.sections.signal())

    fuse_nvm = bench.dev.nvms['Fuses']
    fuse_nvm.dbg_write(80, 0x08)
    fuse_nvm.dbg_write(120, 0x07)
    bench.dev_model.reset()

    bench.sections.fetch_address(100*256)
    assert hook.has_data(SigId.Leave)
    d = hook.pop_data(SigId.Leave)
    assert d == Sections.Boot
    assert hook.has_data(SigId.Enter)
    d = hook.pop_data(SigId.Enter)
    assert d == Sections.AppCode

    bench.sections.fetch_address(150*256)
    assert hook.has_data(SigId.Leave)
    d = hook.pop_data(SigId.Leave)
    assert d == Sections.AppCode
    assert hook.has_data(SigId.Enter)
    d = hook.pop_data(SigId.Enter)
    assert d == Sections.AppData


def test_xt_read_restrictions(bench):
    '''Check that reading a restricted area crashes the device.
    The test method is to make the AppCode section not readable from the Boot section,
    to attempt a read on the flash and check that it crashes the device.
    '''

    #Setting BOOTEND to 100 and APPEND to 0, therefore the AppCode section starts at 0x6400
    fuse_nvm = bench.dev.nvms['Fuses']
    fuse_nvm.dbg_write(100, 0x08)
    fuse_nvm.dbg_write(0, 0x07)
    #Reset to apply the fuse changes and confirm the section start
    bench.dev_model.reset()
    assert bench.sections.section_start(Sections.AppCode) == 100
    #Clear all access flags from the Boot section to the AppCode section
    bench.sections.set_access_flags(Sections.Boot, Sections.AppCode, 0x00)
    #Program the address we want to read to avoid crashing the device for an 'unprogrammed address' error
    bench.dev.nvms['Flash'].spm_write(0xAA, 0x6400)
    #Replace the reset vector by the instruction LDS R0,0xA400 (load direct from data space)
    #the target address is 0xA400 which is mapped to 0x6400 of the flash.
    bench.probe.write_flash(0x0000, bytes([0x00, 0x90, 0x00, 0xA4]))
    #Check that executing the instruction crashes the device
    bench.sim_advance(1, True)
    assert bench.dev_model.state() == corelib.Device.State.Crashed


def test_xt_write_restrictions(bench):
    '''Check that writing a restricted area crashes the device.
    The test method is to make the AppCode section not writeable from the Boot section,
    to attempt a store on the flash and check that it crashes the device.
    '''

    #Setting BOOTEND to 100 and APPEND to 0, therefore the AppCode section starts at 0x6400
    fuse_nvm = bench.dev.nvms['Fuses']
    fuse_nvm.dbg_write(100, 0x08)
    fuse_nvm.dbg_write(0, 0x07)
    #Reset to apply the fuse changes and confirm the section start
    bench.dev_model.reset()
    assert bench.sections.section_start(Sections.AppCode) == 100
    #Remove all access flags from the Boot section to the AppCode section
    bench.sections.set_access_flags(Sections.Boot, Sections.AppCode, 0x00)
    #Replace the reset vector by the instruction STS 0xA400,R0 (store direct to data space)
    #the target address is 0xA400 which is mapped to 0x6400 of the flash.
    bench.probe.write_flash(0x0000, bytes([0x00, 0x92, 0x00, 0xA4]))
    #Check that executing the instruction crashes the device
    bench.sim_advance(1, True)
    assert bench.dev_model.state() == corelib.Device.State.Crashed


def test_xt_fetch_restrictions(bench):
    '''Check that executing an instruction from a non-executable section crashes the device.
    '''

    #Reset to return the PC to 0x000
    bench.dev_model.reset()
    #Set the boot section as non executable
    bench.sections.set_fetch_allowed(Sections.Boot, False)
    #Check that executing the reset vector crashes the device
    bench.sim_advance(1, True)
    assert bench.dev_model.state() == corelib.Device.State.Crashed
