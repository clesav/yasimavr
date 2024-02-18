# test_XT_TCB.py
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
Automatic tests for the Timer type B simulation model (XT architecture)
'''


import pytest
from _test_bench_xt import BenchXT, TESTFW_M4809
from _test_utils import DictSignalHook
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_xt as archlib
from yasimavr.device_library.arch_XT import XT_BaseDevice
from yasimavr.device_library import load_device_from_config, DeviceDescriptor


TMR_CLASS = archlib.ArchXT_TimerB
CFG_CLASS = archlib.ArchXT_TimerBConfig


class _TestDeviceModelTCB(XT_BaseDevice):
    '''The tests use a custom device to ensure all options are enabled
    in the timer/counter model'''

    def __init__(self, dev_descriptor, dev_builder):
        super().__init__(dev_descriptor, dev_builder)
        dev_builder.build_peripherals(self, ['CPUINT', 'SLPCTRL', 'TCA0'])

        tmr_cfg = CFG_CLASS()
        tmr_cfg.reg_base = dev_descriptor.peripherals['TCB0'].reg_base
        tmr_cfg.iv_capt = dev_descriptor.interrupt_map.vectors.index('TCB0')
        tmr_cfg.options = CFG_CLASS.Options.EventCount | CFG_CLASS.Options.OverflowFlag

        self.tmr = TMR_CLASS(0, tmr_cfg)
        self.attach_peripheral(self.tmr)


class _BenchTCB(BenchXT):

    def __init__(self):
        desc = DeviceDescriptor.create_from_model('atmega4809')
        dev_model = load_device_from_config(desc, _TestDeviceModelTCB)

        super().__init__(dev_model, TESTFW_M4809)

        self.sig_capt = corelib.Signal()
        self.sig_count = corelib.Signal()

        ok, reqdata = self.dev_model.ctlreq(corelib.str_to_id('TCB0'), archlib.CTLREQ_TCB_GET_EVENT_HOOK)
        if ok:
            hook_capt = reqdata.data.value(corelib.SignalHook)
            self.sig_capt.connect(hook_capt, TMR_CLASS.CaptureHookTag.Event)
            self.sig_count.connect(hook_capt, TMR_CLASS.CaptureHookTag.Count)

        self.hook_out = DictSignalHook(self.dev.TCB0.signal())

        #per = self.dev_model.find_peripheral('TCB0')
        #per.logger().set_level(corelib.Logger.Level.Debug)

    #Helper to set the capture event input level (0 or 1)
    def set_capture_input(self, value):
        self.sig_capt.raise_(0, value)

    def raise_event_input(self):
        self.sig_count.raise_()

@pytest.fixture
def bench():
    return _BenchTCB()


def test_xt_tcb_start(bench):
    TCB = bench.dev.TCB0

    assert TCB.CTRLA == 0
    assert TCB.CTRLB == 0
    assert TCB.EVCTRL == 0
    assert TCB.INTCTRL == 0
    assert TCB.INTFLAGS == 0
    assert TCB.CNT == 0
    assert TCB.CCMP == 0
    assert TCB.STATUS == 0


def test_xt_tcb_prescaler(bench):
    TCB = bench.dev.TCB0
    TCA = bench.dev.TCA0

    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CCMP = 0xFFFF

    bench.sim_advance(100)
    assert TCB.CNT == 100

    TCB.CTRLA.CLKSEL = 'DIV2'
    bench.sim_advance(100)
    assert TCB.CNT == 150

    #Using TCA as clock source, with a prescaler of 64
    TCB.CTRLA.CLKSEL = 'TCA0'
    TCA.CTRLA.CLKSEL = 'DIV64'
    TCA.CTRLA.ENABLE = 'enabled'
    bench.sim_advance(300)
    assert TCB.CNT == 154


def test_xt_tcb_modeINT(bench):
    TCB = bench.dev.TCB0

    #Initial settings
    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'INT'
    TCB.CCMP = 2000

    #Check that the timer starts counting immediately and that CNT increments
    assert TCB.STATUS.RUN
    bench.sim_advance(100)
    assert TCB.CNT == 100

    #Check that when CNT reaches CCMP, the interrupt flag is not set yet, and the timer keeps running
    bench.sim_advance(1900)
    assert TCB.CNT == 2000
    assert not TCB.INTFLAGS.CAPT
    assert TCB.STATUS.RUN

    #Check that, on the next cyle, CNT wraps at CCMP, the interrupt flag is set and the timer keeps running
    bench.sim_advance(1)
    assert TCB.CNT == 0
    assert TCB.INTFLAGS.CAPT
    assert TCB.STATUS.RUN

    #Check that the interrupt flag is cleared when writing 1 to it
    TCB.INTFLAGS.CAPT = 1
    assert not TCB.INTFLAGS.CAPT

    #Check with a new value of CCMP
    bench.sim_advance(200)
    TCB.CCMP = 100
    bench.sim_advance(100)
    assert TCB.CNT == 300


def test_xt_tcb_modeTIMEOUT(bench):
    TCB = bench.dev.TCB0

    #Initial settings
    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'TIMEOUT'
    TCB.CCMP = 200

    #Check that the timer does not run
    bench.sim_advance(100)
    assert TCB.CNT == 0
    assert not TCB.STATUS.RUN

    #Check that a positive edge on capture input has no effect (CAPTEI is 0)
    bench.set_capture_input(1)
    assert not TCB.STATUS.RUN

    #Check that with CAPTEI = 1,  a positive edge on capture input starts the counting
    bench.set_capture_input(0)
    TCB.EVCTRL.CAPTEI = True
    bench.set_capture_input(1)
    assert TCB.STATUS.RUN

    #Check that the counting stops on a negative edge and does not raise a CAPT event
    #if stopped when CNT < CCMP
    bench.sim_advance(190)
    bench.set_capture_input(0)
    bench.sim_advance(20)
    assert TCB.CNT == 190
    assert not TCB.STATUS.RUN
    assert not TCB.INTFLAGS.CAPT

    #Check that the next pulse on capture input resets the counter and starts a new counting
    bench.set_capture_input(1)
    assert TCB.CNT == 0
    assert TCB.STATUS.RUN

    #Check that the counting stops on a negative edge and raises a CAPT event
    #if stopped when CNT >= CCMP
    bench.sim_advance(210)
    bench.set_capture_input(0)
    bench.sim_advance(20)
    assert TCB.CNT == 210
    assert not TCB.STATUS.RUN
    assert TCB.INTFLAGS.CAPT


def test_xt_tcb_modeCAPT(bench):
    TCB = bench.dev.TCB0

    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'CAPT'
    TCB.EVCTRL.CAPTEI = 'enabled'

    #Check that the counter starts immediately
    assert TCB.STATUS.RUN

    #Check the capture of the counter on a captured positive edge : the counter value is copied
    #into CCMP, an interrupt is raised and the counting carries on.
    bench.sim_advance(200)
    bench.set_capture_input(1)
    bench.sim_advance(100)
    assert TCB.CNT == 300
    assert TCB.STATUS.RUN
    assert TCB.INTFLAGS.CAPT
    assert TCB.CCMP == 200

    #Check that reading CCMP has cleared the interrupt flag
    assert not TCB.INTFLAGS.CAPT


def test_xt_tcb_modeFREQ(bench):
    TCB = bench.dev.TCB0

    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'FRQ'
    TCB.EVCTRL.CAPTEI = 'enabled'

    #Check that the counter starts immediately
    assert TCB.STATUS.RUN

    #Check that the counter restarts on a positive event edge
    bench.sim_advance(100)
    assert TCB.CNT > 0
    bench.set_capture_input(1)
    assert TCB.CNT == 0

    bench.sim_advance(100)
    bench.set_capture_input(0)
    bench.sim_advance(100)
    bench.set_capture_input(1)
    assert TCB.CNT == 0
    assert TCB.INTFLAGS.CAPT
    assert TCB.CCMP == 200

    #Check that reading CCMP has cleared the interrupt flag
    assert not TCB.INTFLAGS.CAPT


def test_xt_tcb_modePW(bench):
    TCB = bench.dev.TCB0

    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'PW'
    TCB.EVCTRL.CAPTEI = 'enabled'

    #Check that the counter starts immediately
    assert TCB.STATUS.RUN

    #Check that a positive edge resets the counter
    bench.sim_advance(200)
    assert TCB.CNT > 0
    bench.set_capture_input(1)
    assert TCB.CNT == 0

    #Check that CNT is copies to CCMP on a negative edge
    bench.sim_advance(100)
    bench.set_capture_input(0)
    bench.sim_advance(100)
    assert TCB.CNT == 200
    assert TCB.INTFLAGS.CAPT
    assert TCB.CCMP == 100

    #Check that reading CCMP has cleared the interrupt flag
    assert not TCB.INTFLAGS.CAPT


def test_xt_tcb_modeFRQPW(bench):
    TCB = bench.dev.TCB0

    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'FRQPW'
    TCB.EVCTRL.CAPTEI = 'enabled'

    #Check that the counter does not start immediately
    bench.sim_advance(500)
    assert not TCB.STATUS.RUN

    #Check that a positive edge starts the counter
    bench.set_capture_input(1)
    assert TCB.STATUS.RUN
    bench.sim_advance(100)
    assert TCB.CNT == 100

    #Check that CNT is copied to CCMP on a negative edge and the next positive edge
    #stops the counter and raises the interrupt flag
    bench.set_capture_input(0)
    bench.sim_advance(200)
    bench.set_capture_input(1)
    bench.sim_advance(100)
    assert not TCB.STATUS.RUN
    assert TCB.CNT == 300
    assert TCB.INTFLAGS.CAPT
    assert TCB.CCMP == 100

    #Check that reading CCMP has cleared the interrupt flag
    assert not TCB.INTFLAGS.CAPT


def test_xt_tcb_modeSINGLE(bench):
    TCB = bench.dev.TCB0

    TCB.CTRLA.CLKSEL = 'DIV1'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CTRLB.CNTMODE = 'SINGLE'
    TCB.EVCTRL.CAPTEI = 'enabled'
    TCB.CTRLB.CCMPEN = 'enabled'
    TCB.CCMP = 100

    #Check that the counter does not start immediately
    assert not TCB.STATUS.RUN

    #Check that an edge starts the counter and sets the output signal to 1
    bench.set_capture_input(1)
    assert TCB.STATUS.RUN
    assert bench.hook_out.has_data(TMR_CLASS.SignalId.Output)
    data = bench.hook_out.pop_data(TMR_CLASS.SignalId.Output)
    assert data.value() == 1

    #Check that the counter stops when CNT reaches CCMP, the interrupt flag is raised
    #and the output signal is set to 0
    bench.sim_advance(100)
    assert TCB.CNT == 100
    assert TCB.STATUS.RUN
    bench.sim_advance(1)
    assert TCB.CNT == 0
    assert not TCB.STATUS.RUN
    assert TCB.INTFLAGS.CAPT
    assert bench.hook_out.has_data(TMR_CLASS.SignalId.Output)
    data = bench.hook_out.pop_data(TMR_CLASS.SignalId.Output)
    assert data.value() == 0


def test_xt_tcb_EventCount(bench):
    TCB = bench.dev.TCB0

    TCB.CTRLA.CLKSEL = 'EVENT'
    TCB.CTRLA.ENABLE = 'enabled'
    TCB.CCMP = 0xFFFF

    #Check that the counter is running but does not count the clock cycles
    bench.sim_advance(100)
    assert TCB.STATUS.RUN
    assert TCB.CNT == 0

    #Check that the counter counts the event signals
    for i in range(5):
        bench.raise_event_input()
        assert TCB.CNT == i + 1
