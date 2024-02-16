# test_io_timer_counter.py
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
Tests for the Timer/Counter generic model
'''

import pytest
import yasimavr.lib.core as corelib
from _test_utils import QueuedSignalHook, DictSignalHook


PT = corelib.PrescaledTimer
TC = corelib.TimerCounter


class _TestTimerSignalHook(QueuedSignalHook):

    def __init__(self, timer, tag):
        super().__init__(timer.signal(), tag)
        self._delay = 0
        self._timer = timer

    def set_timer_delay(self, delay):
        self._delay = delay
        self._timer.set_timer_delay(delay)

    def raised(self, sigdata, tag):
        super().raised(sigdata, tag)
        if sigdata.index == 1:
            self._timer.set_timer_delay(self._delay)


class _BaseBench():

    def __init__(self):
        self.cycler = corelib.CycleManager()
        self.log_handler = corelib.LogHandler()
        self.log_handler.init(self.cycler)
        self.logger = corelib.Logger(0, self.log_handler)
        self.logger.set_level(corelib.Logger.Level.Debug)

    def advance_cycle(self, n):
        end_cycle = self.cycler.cycle() + n
        while self.cycler.cycle() < end_cycle:
            self.cycler.process_timers()

            c = end_cycle
            nw = self.cycler.next_when()
            if nw > 0 and nw < c:
                c = nw
            self.cycler.increment_cycle(c - self.cycler.cycle())


@pytest.fixture
def bench():
    return _BaseBench()


@pytest.fixture
def bch_1tmr(bench):
    tmr = PT()
    bench.log_tmr = corelib.Logger(corelib.str_to_id('TMR'), bench.logger)
    bench.log_tmr.set_level(corelib.Logger.Level.Debug)

    tmr.init(bench.cycler, bench.log_tmr)

    hook = _TestTimerSignalHook(tmr, 0)

    return bench, tmr, hook


@pytest.fixture
def bch_2tmr(bench):
    tmr_a = PT()
    bench.log_ta = corelib.Logger(corelib.str_to_id('TMRA'), bench.logger)
    bench.log_ta.set_level(corelib.Logger.Level.Debug)
    tmr_a.init(bench.cycler, bench.log_ta)

    tmr_b = PT()
    bench.log_tb = corelib.Logger(corelib.str_to_id('TMRB'), bench.logger)
    bench.log_tb.set_level(corelib.Logger.Level.Debug)
    tmr_b.init(bench.cycler, bench.log_tb)

    hook_a = _TestTimerSignalHook(tmr_a, 0)
    hook_b = _TestTimerSignalHook(tmr_b, 1)

    return bench, tmr_a, tmr_b, hook_a, hook_b


def test_core_timer_scheduling(bch_1tmr):
    bench, tmr, hook= bch_1tmr

    bench.advance_cycle(10)

    tmr.set_prescaler(1, 1)
    tmr.set_timer_delay(10)
    bench.advance_cycle(11)
    assert hook.has_data()
    sigdata, _ = hook.pop()
    assert sigdata.data == 10
    assert sigdata.index == 1

    hook.set_timer_delay(20)
    bench.advance_cycle(21)
    assert hook.has_data()


def test_core_timer_chain(bch_2tmr):
    bench, tmr_a, tmr_b, hook_a, hook_b = bch_2tmr

    tmr_a.register_chained_timer(tmr_b)
    tmr_a.set_prescaler(10, 10)
    hook_a.set_timer_delay(10)
    tmr_b.set_prescaler(3, 3)
    hook_b.set_timer_delay(12)

    assert tmr_a.scheduled()
    assert not tmr_b.scheduled()

    bench.advance_cycle(121)
    tmr_a.update()

    sig, _ = hook_a.pop(0)
    assert sig.index == 1
    assert sig.data == 10

    sig, _ = hook_a.pop(0)
    assert sig.index == 0
    assert sig.data == 2

    sig, _ = hook_b.pop(0)
    assert sig.index == 0
    assert sig.data == 3

    hook_a.set_timer_delay(0)

    assert tmr_a.scheduled()
    assert not tmr_b.scheduled()


class _BaseBenchCounter(_BaseBench):

    def __init__(self):
        super().__init__()

        self.ctr = TC(100, 2)
        self.log_ctr = corelib.Logger(corelib.str_to_id('CTR'), self.logger)
        self.log_ctr.set_level(corelib.Logger.Level.Debug)
        self.ctr.init(self.cycler, self.log_ctr)
        self.ctr.prescaler().set_prescaler(1, 1)

        self.hk = DictSignalHook(self.ctr.signal(), 0)

    def advance_cycle(self, ncycles):
        self.ctr.reschedule()
        super().advance_cycle(ncycles)
        self.ctr.update()


@pytest.fixture
def bch_ctr():
    b = _BaseBenchCounter()
    return b, b.ctr, b.hk


def test_core_counter_source(bch_ctr):
    #Check that , in Timer source, the counter progresses with
    #the cycle advance

    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Stopped)
    bench.advance_cycle(10)
    assert ctr.counter() == 0

    ctr.set_tick_source(TC.TickSource.Timer)
    bench.advance_cycle(10)
    assert ctr.counter() == 10

    ctr.set_tick_source(TC.TickSource.Stopped)
    bench.advance_cycle(10)
    assert ctr.counter() == 10

    ctr.set_tick_source(TC.TickSource.Timer)
    bench.advance_cycle(10)
    assert ctr.counter() == 20


def test_core_counter_ext_source(bch_ctr):
    #Check that, in External source, the counter does not progress with
    #the timer but is increment each time the ext tick hook is raised.

    bench, ctr, hook = bch_ctr

    sig = corelib.Signal()
    sig.connect(ctr.ext_tick_hook())

    ctr.set_tick_source(TC.TickSource.External)
    bench.advance_cycle(10)
    assert ctr.counter() == 0

    for i in range(10):
        sig.raise_()
        assert ctr.counter() == i + 1


def test_core_counter_top(bch_ctr):
    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Timer)
    ctr.set_top(50)
    bench.advance_cycle(51)
    assert ctr.counter() == 50
    bench.advance_cycle(1)
    assert ctr.counter() == 0

    f = hook.pop_data(TC.SignalId.Event).value()
    assert f & TC.EventType.Top

    bench.advance_cycle(10)
    assert ctr.counter() == 10


def test_core_counter_bottom(bch_ctr):
    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Timer)
    ctr.set_top(50)

    assert not hook.has_data(TC.SignalId.Event)

    bench.advance_cycle(1)
    bench.cycler.process_timers()

    f = hook.pop_data(TC.SignalId.Event).value()
    assert f & TC.EventType.Bottom


def test_core_counter_compare(bch_ctr):
    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Timer)
    ctr.set_top(50)
    ctr.set_comp_value(0, 20)
    ctr.set_comp_value(1, 30)

    bench.advance_cycle(21)
    assert not hook.has_data(TC.SignalId.CompMatch)

    ctr.set_counter(0)
    ctr.set_comp_enabled(0, True)
    ctr.set_comp_enabled(1, True)

    bench.advance_cycle(20)
    assert not hook.has_data(TC.SignalId.CompMatch)

    bench.advance_cycle(1)
    assert hook.has_data(TC.SignalId.CompMatch, 0)
    sig, _ = hook.pop(TC.SignalId.CompMatch, 0)
    assert sig.data.value() & TC.EventType.Compare

    bench.advance_cycle(10)

    assert hook.has_data(TC.SignalId.CompMatch, 1)
    sig, _ = hook.pop(TC.SignalId.CompMatch, 1)
    assert sig.data.value() & TC.EventType.Compare

    bench.advance_cycle(20)
    assert hook.has_data(TC.SignalId.Event)
    assert hook.pop_data(TC.SignalId.Event).value() & TC.EventType.Top


def test_core_counter_countdown(bch_ctr):
    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Timer)
    ctr.set_top(50)
    ctr.set_slope_mode(TC.SlopeMode.Down)

    assert ctr.countdown()

    bench.advance_cycle(1)
    bench.cycler.process_timers()
    assert ctr.counter() == 50


def test_core_counter_doubleslope(bch_ctr):
    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Timer)
    ctr.set_top(9)
    ctr.set_slope_mode(TC.SlopeMode.Double)

    bench.advance_cycle(5)
    assert not ctr.countdown()

    bench.advance_cycle(10)
    assert ctr.countdown()

    bench.advance_cycle(10)
    assert not ctr.countdown()


def test_core_counter_special_case(bch_ctr):
    bench, ctr, hook = bch_ctr

    ctr.set_tick_source(TC.TickSource.Timer)
    ctr.set_top(0)

    bench.advance_cycle(1)
    assert ctr.counter() == 0

    ctr.set_slope_mode(TC.SlopeMode.Double)
    bench.advance_cycle(1)
    assert ctr.counter() == 0

    ctr.set_slope_mode(TC.SlopeMode.Down)
    bench.advance_cycle(1)
    assert ctr.counter() == 0
