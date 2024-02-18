# test_XT_TCA.py
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
Tests for the Timer type A simulation model (XT architecture)
'''


import pytest
from _test_bench_xt import BenchXT, TESTFW_M4809
from _test_utils import DictSignalHook
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_xt as archlib
from yasimavr.device_library.arch_XT import XT_BaseDevice
from yasimavr.device_library import load_device_from_config, DeviceDescriptor


TMR_CLASS = archlib.ArchXT_TimerA
CFG_CLASS = archlib.ArchXT_TimerAConfig


class _TestDeviceModelTCA(XT_BaseDevice):
    '''Simple device model building the bare minimum peripherals
    '''

    def __init__(self, dev_descriptor, dev_builder):
        super().__init__(dev_descriptor, dev_builder)
        dev_builder.build_peripherals(self, ['CPUINT', 'SLPCTRL'])

        tmr_cfg = CFG_CLASS()
        tmr_cfg.reg_base = dev_descriptor.peripherals['TCA0'].reg_base
        tmr_cfg.iv_ovf = dev_descriptor.interrupt_map.vectors.index('TCA0_OVF')
        tmr_cfg.iv_hunf = dev_descriptor.interrupt_map.vectors.index('TCA0_HUNF')

        iv_cmp = [dev_descriptor.interrupt_map.vectors.index('TCA0_CMP%d' % i)
                  for i in range(CFG_CLASS.CompareChannelCount)]
        tmr_cfg.ivs_cmp = iv_cmp

        self.tmr = TMR_CLASS(tmr_cfg)
        self.attach_peripheral(self.tmr)


class _BenchTCA(BenchXT):

    def __init__(self):
        desc = DeviceDescriptor.create_from_model('atmega4809')
        dev_model = load_device_from_config(desc, _TestDeviceModelTCA)

        super().__init__(dev_model, TESTFW_M4809)

        self.sig_eva = corelib.Signal()
        self.sig_evb = corelib.Signal()
        ok, reqdata = self.dev_model.ctlreq(corelib.str_to_id('TCA0'), archlib.CTLREQ_TCA_GET_EVENT_HOOK)
        if ok:
            self.event_hook = reqdata.data.value(corelib.SignalHook)
            self.sig_eva.connect(self.event_hook, TMR_CLASS.EventHookTag.EventA)
            self.sig_evb.connect(self.event_hook, TMR_CLASS.EventHookTag.EventB)

        self.wfm_hook = DictSignalHook(self.dev.TCA0.signal())


    def raise_event_input(self, tag, level):
        if tag == 'A':
            self.sig_eva.raise_(0, level)
        elif tag == 'B':
            self.sig_evb.raise_(0, level)

@pytest.fixture
def bench():
    return _BenchTCA()


def test_xt_tca_start(bench):
    '''Check the value of the registers on start
    '''

    TCA= bench.dev.TCA0

    assert TCA.CTRLA == 0
    assert TCA.CTRLB == 0
    assert TCA.CTRLC == 0
    assert TCA.CTRLESET == 0
    assert TCA.CTRLECLR == 0
    assert TCA.CTRLFSET == 0
    assert TCA.CTRLFCLR == 0
    assert TCA.EVCTRL == 0
    assert TCA.INTCTRL == 0
    assert TCA.INTFLAGS == 0
    assert TCA.CNT == 0
    assert TCA.PER == 0XFFFF
    assert TCA.PERBUF == 0XFFFF
    assert TCA.CMP0 == 0
    assert TCA.CMP0BUF == 0
    assert TCA.CMP1 == 0
    assert TCA.CMP1BUF == 0
    assert TCA.CMP2 == 0
    assert TCA.CMP2BUF == 0


def test_xt_tca_direction(bench):
    '''Check the control of the direction of counting
    '''

    TCA= bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLA.CLKSEL = 'DIV1'
    TCA.CNT = 100

    #Check that the counter counts up by default
    bench.sim_advance(10)
    assert TCA.CNT == 110

    #Check that the counter counts down with DIR = 1
    TCA.CTRLESET.DIR = 1
    bench.sim_advance(20)
    assert TCA.CNT == 90
    assert TCA.CTRLESET.DIR
    assert TCA.CTRLECLR.DIR

    #Check that writing to CTRLECLR clears the DIR bit
    TCA.CTRLECLR.DIR = 1
    bench.sim_advance(30)
    assert TCA.CNT == 120
    assert not TCA.CTRLESET.DIR
    assert not TCA.CTRLECLR.DIR


def test_xt_tca_buffers(bench):
    '''Test of the counter and compare channel buffers
    '''

    TCA= bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLA.CLKSEL = 'DIV1'
    TCA.PERBUF = 0x4444
    TCA.CMP0BUF = 0x1111
    TCA.CMP1BUF = 0x2222
    TCA.CMP2BUF = 0x3333

    assert TCA.PER == 0xFFFF
    assert TCA.CMP0 == 0
    assert TCA.CMP1 == 0
    assert TCA.CMP2 == 0
    assert TCA.CTRLFSET.CMP0BV
    assert TCA.CTRLFSET.CMP1BV
    assert TCA.CTRLFSET.CMP2BV

    #Check that the registers have been updated with the buffers at TOP
    bench.sim_advance(0x10000)
    assert TCA.PER == 0x4444
    assert TCA.CMP0 == 0x1111
    assert TCA.CMP1 == 0x2222
    assert TCA.CMP2 == 0x3333
    assert not TCA.CTRLFSET.CMP0BV
    assert not TCA.CTRLFSET.CMP1BV
    assert not TCA.CTRLFSET.CMP2BV


def test_xt_tca_normal0(bench):
    '''Check the normal waveform mode
    '''

    TCA= bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLA.CLKSEL = 'DIV1'
    TCA.PER = 1000

    bench.sim_advance(1001)
    assert TCA.CNT == 0
    assert TCA.INTFLAGS.OVF
    TCA.INTFLAGS.OVF = 1
    assert not TCA.INTFLAGS.OVF

    bench.sim_advance(1000)
    assert TCA.CNT == 1000
    assert not TCA.INTFLAGS.OVF
    bench.sim_advance(1)
    assert TCA.CNT == 0
    assert TCA.INTFLAGS.OVF
    TCA.INTFLAGS.OVF = 1

    bench.sim_advance(500)
    assert TCA.CNT == 500
    assert not TCA.INTFLAGS.OVF
    bench.sim_advance(501)
    assert TCA.CNT == 0
    assert TCA.INTFLAGS.OVF
    TCA.INTFLAGS.OVF = 1

    TCA.CMP0 = 500
    bench.sim_advance(500)
    assert TCA.CNT == 500
    assert not TCA.INTFLAGS.CMP0

    bench.sim_advance(1)
    assert TCA.INTFLAGS.CMP0


def test_xt_tca_freqgen(bench):
    '''Check the frequency generation mode
    '''

    TCA= bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLB.WGMODE = 'FRQ'
    TCA.CTRLB.CMP1EN = 'enabled'
    TCA.CTRLB.CMP0EN = 'enabled'
    TCA.CMP0 = 1000
    TCA.CMP1 = 500

    bench.sim_advance(501)
    assert not TCA.CTRLC.CMP0OV
    assert TCA.CTRLC.CMP1OV
    assert TCA.CNT == 501
    assert bench.wfm_hook.has_data(TMR_CLASS.SignalId.CompareOutput, 1)
    assert bench.wfm_hook.pop_data(TMR_CLASS.SignalId.CompareOutput, 1) == 1

    bench.sim_advance(500)
    assert TCA.CTRLC.CMP0OV
    assert TCA.CTRLC.CMP1OV
    assert TCA.CNT == 0
    assert bench.wfm_hook.has_data(TMR_CLASS.SignalId.CompareOutput, 0)
    assert bench.wfm_hook.pop_data(TMR_CLASS.SignalId.CompareOutput, 0) == 1

    bench.sim_advance(501)
    assert TCA.CTRLC.CMP0OV
    assert not TCA.CTRLC.CMP1OV


def test_xt_tca_singleslope_pwm(bench):
    '''Test of the single slope PWM mode
    '''

    TCA= bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLB.WGMODE = 'SINGLESLOPE'
    TCA.PER = 1000
    TCA.CMP0 = 200

    bench.sim_advance(2)
    assert TCA.CTRLC.CMP0OV

    bench.sim_advance(100)
    #Check that the direction bit cannot be set
    TCA.CTRLESET.DIR = 'down'
    assert TCA.CTRLESET.DIR == 'up'

    bench.sim_advance(200)
    assert not TCA.CTRLC.CMP0OV

    TCA.CMP0 = 0
    TCA.CNT = 0
    bench.sim_advance(1)
    assert not TCA.CTRLC.CMP0OV

    TCA.CMP0 = 1100
    TCA.CNT = 0
    bench.sim_advance(1001)
    assert TCA.CTRLC.CMP0OV


def test_xt_tca_doubleslope_pwm(bench):
    '''Test of the double slope PWM mode
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLB.WGMODE = 'DSTOP'
    TCA.PER = 1000
    TCA.CMP0 = 200

    bench.sim_advance(2)
    assert TCA.CTRLC.CMP0OV

    bench.sim_advance(200)
    assert not TCA.CTRLC.CMP0OV

    bench.sim_advance(799)
    assert TCA.CNT == 999
    assert TCA.CTRLESET.DIR
    assert not TCA.CTRLC.CMP0OV

    bench.sim_advance(800)
    assert TCA.CNT == 199
    assert TCA.CTRLC.CMP0OV

    TCA.CMP0 = 0
    TCA.CNT = 0
    bench.sim_advance(2)
    assert not TCA.CTRLC.CMP0OV


def test_xt_tca_cmd_restart(bench):
    '''Test of the RESTART command
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.PER = 1000

    bench.sim_advance(10)
    TCA.CTRLESET.DIR = 'down'
    TCA.CTRLESET.CMD = 'RESTART'

    assert TCA.CTRLESET.CMD == 'NONE'
    assert TCA.CTRLESET.DIR == 'up'
    assert TCA.CNT == 0


def test_xt_tca_cmd_update(bench):
    '''Test of the UPDATE command
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.PER = 1000
    TCA.PERBUF = 2000
    assert TCA.PER == 1000
    assert TCA.CTRLFCLR.PERBV

    TCA.CTRLESET.CMD = 'UPDATE'
    assert TCA.PER == 2000
    assert not TCA.CTRLFCLR.PERBV


def test_xt_tca_cmd_reset(bench):
    '''Test of the RESET command
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.PER = 1000

    TCA.CTRLESET.CMD = 'RESET'
    assert TCA.CTRLA.ENABLE == 'disabled'
    assert TCA.PER == 0xFFFF


def test_xt_tca_lock_update(bench):
    '''test of the LUPD bit feature
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLESET.LUPD = 1
    TCA.CMP0BUF = 0x1111

    TCA.CTRLESET.CMD = 'UPDATE'
    assert TCA.CTRLFSET.CMP0BV
    assert TCA.CMP0 == 0

    TCA.CTRLECLR.LUPD = 1
    TCA.CTRLESET.CMD = 'UPDATE'
    assert not TCA.CTRLFSET.CMP0BV
    assert TCA.CMP0 == 0x1111


def test_xt_tca_auto_lock_update(bench):
    '''Test of the ALUPD bit feature
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.CTRLB.ALUPD = 1
    TCA.CMP0BUF = 0x1111
    TCA.CMP1BUF = 0x2222
    TCA.CMP2BUF = 0x3333

    assert TCA.CTRLESET.LUPD == 'locked'

    TCA.CTRLESET.CMD = 'UPDATE'
    assert TCA.CMP0 == 0

    TCA.PERBUF = 0x4444
    assert TCA.CTRLESET.LUPD == 'unlocked'

    TCA.CTRLESET.CMD = 'UPDATE'
    assert TCA.CMP0 == 0x1111


def test_xt_tca_event_counting(bench):
    '''Test of the event counting
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLA.ENABLE = 'enabled'
    TCA.EVCTRL.CNTAEI = 1

    bench.sim_advance(10)
    assert TCA.CNT == 0

    bench.raise_event_input('A', 1)
    assert TCA.CNT == 1

    bench.raise_event_input('A', 0)
    assert TCA.CNT == 1

    bench.raise_event_input('A', 1)
    assert TCA.CNT == 2

    TCA.EVCTRL.EVACTA = 'CNT_HIGHLVL'
    bench.sim_advance(10)
    assert TCA.CNT == 12

    bench.raise_event_input('A', 0)
    bench.sim_advance(10)
    assert TCA.CNT == 12

    TCA.EVCTRL.EVACTA = 'UPDOWN'
    bench.sim_advance(10)
    assert TCA.CNT == 22

    bench.raise_event_input('A', 1)
    bench.sim_advance(5)
    assert TCA.CNT == 17


def test_xt_tca_split_mode(bench):
    '''Test of the split mode
    '''

    TCA = bench.dev.TCA0

    TCA.CTRLD.SPLITM = 'SPLIT'
    TCA.CTRLA.ENABLE = 'enabled'
    TCA.LPER = 60
    TCA.HPER = 100

    bench.sim_advance(40)
    assert TCA.LCNT == 21
    assert TCA.HCNT == 61

    bench.sim_advance(40)
    assert TCA.LCNT == 42
    assert TCA.HCNT == 21


if __name__ == "__main__":
    unittest.main()
