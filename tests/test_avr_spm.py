# test_avr_nvm.py
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

'''
Tests for the Self-Programming function in AVR architecture
'''

import pytest
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_avr as archlib
from _test_bench_avr import BenchAVR, PATH_TESTFW_M328, bench_m328
from yasimavr.utils.sim_dump import sim_dump
from yasimavr.device_library.arch_AVR import AVR_BaseDevice
from yasimavr.device_library import load_device_from_config, DeviceDescriptor


class _DummyPeripheral(corelib.Peripheral):

    def __init__(self):
        super().__init__(corelib.IOCTL_NVM)
        self.nvm_req = None

    def ctlreq(self, reqid, reqdata):
        if reqid == corelib.CTLREQ_NVM_REQUEST:
            self.nvm_req = corelib.NVM_request_t(reqdata.data.value(corelib.NVM_request_t))
            return True
        return super().ctlreq(reqid, reqdata)


class _DummyTestDevice(AVR_BaseDevice):

    def __init__(self, dev_descriptor, builder):
        super().__init__(dev_descriptor, builder)

        builder.build_peripherals(self, ['CPUINT', 'SLPCTRL'])

        self.dummy = _DummyPeripheral()
        self.attach_peripheral(self.dummy)


@pytest.fixture
def dummy_spm_bench():
    desc = DeviceDescriptor.create_from_model('atmega328')
    dev_model = load_device_from_config(desc, _DummyTestDevice)
    return BenchAVR(dev_model, PATH_TESTFW_M328)


@pytest.fixture
def bench():
    b = bench_m328()
    b.dev_model.find_peripheral('NVM').logger().set_level(corelib.Logger.Level.Debug)
    return b


SPM_OPCODE = bytes((0xE8, 0x95))

def _send_SPM_request(dev_model, addr, data):
    nvm_req = corelib.NVM_request_t()
    nvm_req.kind = 0
    nvm_req.addr = addr
    nvm_req.data = data
    nvm_req.result = 0
    ctlreq = corelib.ctlreq_data_t()
    ctlreq.data.set(nvm_req)
    dev_model.ctlreq(corelib.IOCTL_NVM, corelib.CTLREQ_NVM_REQUEST, ctlreq)
    return nvm_req


def test_avr_spm_write_request(dummy_spm_bench):
    '''Check that when a SPM instruction is executed, a IOCTL_NVM_REQUEST is sent to the SPM peripheral
    '''

    bench = dummy_spm_bench
    core = bench.dev.core
    #Write the SPM opcode in a empty flash address and set the Program Counter to it
    bench.dev.nvms['Flash'].program(SPM_OPCODE, 0x7000)
    core.PC = 0x7000
    #Set the Z register to the address to write
    core.RZ = 0x0500
    #Set R0+R1 to the data to write
    core.R0 = 0xAA
    core.R1 = 0x55
    #Wakeup the device and execute the instruction
    bench.probe.set_device_state(corelib.Device.State.Running)
    bench.sim_advance(1)
    #Check that the dummy peripheral received a NVM request
    nvm_req = bench.dev_model.dummy.nvm_req
    assert nvm_req is not None
    #Check the request parameters
    assert nvm_req.kind == 0
    assert nvm_req.addr == 0x500
    assert nvm_req.data == 0x55AA


def test_avr_spm_write_1(bench):
    '''Check that executing SPM when no SPM operation is enabled
    leads to a device crash or is ignored, depending on the device option
    IgnoreBadCpuLPM
    '''

    bench.dev.core.PC = 0x07F00

    bench.dev_model.set_option(corelib.Device.Option.IgnoreBadCpuLPM, True)
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert bench.dev_model.state() == corelib.Device.State.Sleeping

    bench.dev_model.set_option(corelib.Device.Option.IgnoreBadCpuLPM, False)
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert bench.dev_model.state() == corelib.Device.State.Crashed


def test_avr_spm_write_2(bench):
    '''Check that executing SPM from an application section
    leads to a device crash or is ignored, depending on the device option
    IgnoreBadCpuLPM
    '''

    bench.dev.NVMCTRL.SPMCSR = 0x01
    bench.dev_model.set_option(corelib.Device.Option.IgnoreBadCpuLPM, True)
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert bench.dev_model.state() == corelib.Device.State.Sleeping

    bench.dev_model.set_option(corelib.Device.Option.IgnoreBadCpuLPM, False)
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert bench.dev_model.state() == corelib.Device.State.Crashed


def test_avr_spm_write_3(bench):
    '''Check that the CPU is halted during a flash write
    Check that the flash is writting and/or erased according to the SPM operation
    '''
    
    flash = bench.dev.nvms['Flash']
    NVMCTRL = bench.dev.NVMCTRL

    bench.dev.core.PC = 0x07F00

    #Check that the SPMCR=0x01 has no effect
    NVMCTRL.SPMCSR = 0x01
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert nvm_req.result > 0
    assert not NVMCTRL.SPMCSR.read()
    assert bench.dev_model.state() == corelib.Device.State.Sleeping
    assert not flash.programmed(0x7000)

    #Check that the SPMCR=0x05 programs the flash
    NVMCTRL.SPMCSR = 0x05
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert nvm_req.result > 0
    assert bench.dev_model.state() == corelib.Device.State.Halted
    bench.sim_advance(4600)
    assert bench.dev_model.state() == corelib.Device.State.Sleeping
    assert not NVMCTRL.SPMCSR.read()
    assert flash[0x7000] == 0xAA
    assert flash[0x7001] == 0x55

    #Check that the SPMCR=0x03 erases the flash
    NVMCTRL.SPMCSR = 0x03
    nvm_req = _send_SPM_request(bench.dev_model, 0x7000, 0x55AA)
    assert nvm_req.result > 0
    assert bench.dev_model.state() == corelib.Device.State.Halted
    bench.sim_advance(4600)
    assert bench.dev_model.state() == corelib.Device.State.Sleeping
    assert not NVMCTRL.SPMCSR.read()
    assert not flash.programmed(0x7000) and not flash.programmed(0x7001)



def test_avr_eeprom_read(bench):
    '''Check the read operation on the EEPROM
    '''

    NVMCTRL = bench.dev.NVMCTRL
    for i, c in enumerate(b'EEPROM\0'):
        NVMCTRL.EECR.EEMPE = 1
        NVMCTRL.EEAR = i
        NVMCTRL.EECR.EERE = 1
        assert not NVMCTRL.EECR.EEMPE
        assert not NVMCTRL.EECR.EERE
        assert NVMCTRL.EEDR == c


def test_avr_eeprom_write_enable(bench):
    '''Check the behaviour of the EEMPE bit
    '''

    NVMCTRL = bench.dev.NVMCTRL
    NVMCTRL.EECR.EEMPE = 1
    for i in range(5):
        assert NVMCTRL.EECR.EEMPE == 1
        bench.sim_advance(1)

    assert NVMCTRL.EECR.EEMPE == 0


def test_avr_eeprom_write(bench):
    '''Check the erase/write operations on the EEPROM
    '''

    eeprom = bench.dev.nvms['EEPROM']
    NVMCTRL = bench.dev.NVMCTRL

    NVMCTRL.EEAR = 0
    NVMCTRL.EECR.EEPE = 1
    assert NVMCTRL.EECR.EEPE == 0

    #Check the erase/write operation
    NVMCTRL.EEDR = 0x53 #it should read 'S'
    NVMCTRL.EECR.EEMPE = 1
    NVMCTRL.EECR.EEPE = 1
    assert NVMCTRL.EECR.EEPE == 1
    assert NVMCTRL.EECR.EEMPE == 0
    bench.sim_advance(4000)
    assert NVMCTRL.EECR.EEPE == 0
    assert bytes(eeprom.dbg_read(0, 7)) == b'SEPROM\0'

    #Check the erase operation
    NVMCTRL.EECR.EEPM = 1
    NVMCTRL.EECR.EEMPE = 1
    NVMCTRL.EECR.EEPE = 1
    bench.sim_advance(4000)
    assert NVMCTRL.EECR.EEPE == 0
    assert not eeprom.programmed(0)

    #Check the write operation
    NVMCTRL.EEDR = 0x55
    NVMCTRL.EECR.EEPM = 2
    NVMCTRL.EECR.EEMPE = 1
    NVMCTRL.EECR.EEPE = 1
    bench.sim_advance(4000)
    assert NVMCTRL.EECR.EEPE == 0
    assert eeprom.programmed(0)
    assert eeprom[0] == 0x55

    #Check the overwrite operation - the result is a AND-ed value
    NVMCTRL.EEDR = 0xAA
    NVMCTRL.EECR.EEPM = 2
    NVMCTRL.EECR.EEMPE = 1
    NVMCTRL.EECR.EEPE = 1
    bench.sim_advance(4000)
    assert eeprom[0] == 0x00
