# test_avr_wdt.py
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
from _test_bench_avr import bench_m328
from _test_utils import PinState, DictSignalHook
from yasimavr.lib.core import Logger, Device


'''
Test of the watchdog timer on ATMega328
'''


@pytest.fixture
def bench():
    return bench_m328()


def test_avr_watchdog_interrupt(bench):
    WDTCSR = bench.dev.WDT.WDTCSR
    MCUCSR = bench.dev.RSTCTRL.MCUSR
    CPU = bench.dev.CPU

    per_wdt = bench.dev_model.find_peripheral('WDT')
    per_wdt.logger().set_level(Logger.Level.Debug)

    WDTCSR.WDIE = 1
    CPU.SREG.I = 0
    assert not WDTCSR.WDIF
    bench.sim_advance(16000) #16ms
    assert WDTCSR.WDIF
    assert WDTCSR.WDIE
    assert not MCUCSR.WDRF

    CPU.SREG.I = 1
    bench.sim_advance(100)
    assert not WDTCSR.WDIF
    assert not WDTCSR.WDIE
    assert CPU.GPIOR0 == 6


def test_avr_watchdog_change_protection(bench):
    WDTCSR = bench.dev.WDT.WDTCSR

    WDTCSR.WDP = '4K'
    assert WDTCSR.WDP == '2K'

    with WDTCSR:
        WDTCSR.WDCE = 1
        WDTCSR.WDP = '4K'
    assert WDTCSR.WDP == '2K'

    with WDTCSR:
        WDTCSR.WDCE = 1
        WDTCSR.WDE = 1
    WDTCSR.WDP = '4K'
    assert WDTCSR.WDP == '4K'

    bench.sim_advance(2)
    with WDTCSR:
        WDTCSR.WDCE = 0
        WDTCSR.WDP = '8K'
    assert WDTCSR.WDCE
    assert WDTCSR.WDP == '8K'

    bench.sim_advance(3)
    with WDTCSR:
        WDTCSR.WDCE = 0
        WDTCSR.WDP = '2K'
    assert not WDTCSR.WDCE
    assert WDTCSR.WDP == '8K'


def test_avr_watchdog_timeout(bench):
    WDTCSR = bench.dev.WDT.WDTCSR
    MCUSR = bench.dev.RSTCTRL.MCUSR

    with WDTCSR:
        WDTCSR.WDCE = 1
        WDTCSR.WDE = 1

    with WDTCSR:
        WDTCSR.WDE = 1
        WDTCSR.WDP = '2K'
        WDTCSR.WDIE = 1

    bench.sim_advance(15001)
    assert WDTCSR.WDIF

    bench.sim_advance(1)
    assert not WDTCSR.WDIF
    assert not WDTCSR.WDIE
    assert WDTCSR.WDE

    bench.sim_advance(16000)
    assert MCUSR.WDRF
