# test_core_cycles.py
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


import yasimavr.lib.core as corelib


class _TestCycleTimer(corelib.CycleTimer):

    def __init__(self):
        super().__init__()
        self.called = -1

    def next(self, when):
        self.called = when


def test_cycle_timer_getters():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()

    assert tmr.manager() == None

    tmr.init(mgr)
    assert tmr.manager() == mgr

    assert not tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() < 0

    tmr.delay(10)
    assert tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() == 10
    assert mgr.next_when() == 10

    tmr.pause()
    assert tmr.scheduled()
    assert tmr.paused()
    assert tmr.remaining_delay() == 10
    assert mgr.next_when() < 0

    tmr.resume()
    assert tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() == 10
    assert mgr.next_when() == 10

    tmr.cancel()
    assert not tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() < 0


def test_cycle_next_when():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()
    tmr.init(mgr)

    assert mgr.next_when() == -1

    tmr.delay(10)
    assert mgr.next_when() == 10

    mgr.increment_cycle(10)
    mgr.process_timers()
    assert mgr.next_when() == -1
    assert tmr.called == 10


def test_cycle_ref_freq():
    mgr = corelib.CycleManager()

    assert mgr.reference_frequency() == 1.0

    mgr.increment_cycle(10)
    assert mgr.elapsed_time() == 10.0

    mgr.configure_clock_source(0, 2.0)
    mgr.increment_cycle(10)

    assert mgr.reference_frequency() == 2.0
    assert mgr.elapsed_time() == 15.0

    mgr.configure_clock_source(0, 0.5)
    mgr.increment_cycle(10)

    assert mgr.reference_frequency() == 0.5
    assert mgr.elapsed_time() == 35.0

    mgr.set_direct_freq(2.0)
    assert mgr.reference_frequency() == 2.0

    mgr.set_direct_freq(0.0)
    assert mgr.reference_frequency() == 0.5


def test_cycle_delay_secs():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()
    tmr.init(mgr)

    mgr.configure_clock_source(0, 2.0)
    tmr.delay_s(10.0)
    mgr.increment_cycle(20)
    mgr.process_timers()
    assert tmr.called == 20


def test_cycle_domain():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()
    tmr.init(mgr, 1)
    mgr.add_clock_domain(1)

    tmr.delay(10)
    assert tmr.remaining_delay() == 10

    mgr.configure_clock_domain(1, 0, 2)
    assert mgr.domain_frequency(1) == 0.5
    assert mgr.next_when() == 20

    mgr.configure_clock_domain(1, 0, 4)
    assert mgr.domain_frequency(1) == 0.25
    assert mgr.reference_frequency() == 1.0
    assert mgr.next_when() == 40

    mgr.increment_cycle(10)
    assert tmr.remaining_delay() == 8

    tmr.delay(15)
    assert tmr.remaining_delay() == 15
    assert mgr.next_when() == 70
