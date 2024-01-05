# test_core_cycles.py
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


import yasimavr.lib.core as corelib


class _TestCycleTimer(corelib.CycleTimer):

    def __init__(self):
        super().__init__()
        self.called = -1
        self.next_when = 0

    def next(self, when):
        self.called = when
        return self.next_when


def test_cycle_timer_getters():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()

    assert not tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() < 0

    mgr.schedule(tmr, 10)
    assert tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() == 10
    assert mgr.next_when() == 10

    mgr.pause(tmr)
    assert tmr.scheduled()
    assert tmr.paused()
    assert tmr.remaining_delay() == 10
    assert mgr.next_when() == -1

    mgr.resume(tmr)
    assert tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() == 10
    assert mgr.next_when() == 10

    mgr.cancel(tmr)
    assert not tmr.scheduled()
    assert not tmr.paused()
    assert tmr.remaining_delay() == -1


def test_cycle_next_when():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()

    assert mgr.next_when() == -1

    mgr.schedule(tmr, 10)
    assert mgr.next_when() == 10

    mgr.increment_cycle(10)
    mgr.process_timers()
    assert mgr.next_when() == -1


def test_cycle_process():
    mgr = corelib.CycleManager()
    tmr = _TestCycleTimer()
    tmr.next_when = 20
    mgr.schedule(tmr, 10)

    mgr.increment_cycle(5)
    assert tmr.remaining_delay() == 5

    mgr.increment_cycle(10)
    assert tmr.scheduled()
    assert tmr.remaining_delay() == 0

    mgr.process_timers()
    assert tmr.called == 10
    assert mgr.next_when() == 20
    assert tmr.scheduled()

    tmr.next_when = 0
    mgr.increment_cycle(10)
    mgr.process_timers()
    assert tmr.called == 20
    assert mgr.next_when() == -1
    assert not tmr.scheduled()
