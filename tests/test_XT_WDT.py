# test_XT_WDT.py
#
# Copyright 2026 Clement Savergne <csavergne@yahoo.com>
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
from _test_bench_xt import bench_m4809
from yasimavr.lib.core import CTLREQ_WATCHDOG_RESET


'''
Test of the watchdog timer on ATMega4809
'''


@pytest.fixture
def bench():
    return bench_m4809()


def test_xt_watchdog_period(bench):
    WDT = bench.dev.WDT
    RSTCTRL = bench.dev.RSTCTRL
    MISCREG = bench.dev.MISC

    bench.sim_advance(1000000)
    assert not RSTCTRL.RSTFR.WDRF

    MISCREG.GPIOR0 = 0xAA
    WDT.CTRLA.PERIOD = 0x01
    bench.sim_advance(7814) #8CLK * F_CPU (1e6Hz) / F_WDT (1024Hz) rounded up + 1 cycle
    #Check that the device has not been reset yet
    assert not RSTCTRL.RSTFR.WDRF
    assert bench.probe.read_pc() != 0x0000
    assert MISCREG.GPIOR0 == 0xAA
    bench.sim_advance(1)
    #Check that the device has been reset (PC == 0 and register GPIOR0 cleared) and the WDRF flag set
    assert RSTCTRL.RSTFR.WDRF
    assert bench.probe.read_pc() == 0x0000
    assert MISCREG.GPIOR0 == 0x00


def test_xt_watchdog_window(bench):
    WDT = bench.dev.WDT
    RSTCTRL = bench.dev.RSTCTRL
    MISCREG = bench.dev.MISC

    with WDT.CTRLA:
        WDT.CTRLA.PERIOD = 0x01 #8CLK
        WDT.CTRLA.WINDOW = 0x01 #8CLK

    MISCREG.GPIOR0 = 0xAA

    bench.sim_advance(1000)
    assert MISCREG.GPIOR0 == 0xAA

    #Issue a WDR and wait for synchronisation (3 * F_CPU / F_WDT ~ 3000 cycles)
    bench.dev_model.ctlreq('WDT', CTLREQ_WATCHDOG_RESET)
    bench.sim_advance(3000)

    #Check that the device has not been reset (the window is ignored for the first WDR)
    assert MISCREG.GPIOR0 == 0xAA
    assert not RSTCTRL.RSTFR.WDRF

    #Issue a WDR after 8ms
    bench.sim_advance(8000)
    bench.dev_model.ctlreq('WDT', CTLREQ_WATCHDOG_RESET)
    bench.sim_advance(3000)

    #Check that the device has not been reset (WDR within the window)
    assert MISCREG.GPIOR0 == 0xAA
    assert not RSTCTRL.RSTFR.WDRF

    #Issue a WDR after 1ms
    bench.sim_advance(1000)
    bench.dev_model.ctlreq('WDT', CTLREQ_WATCHDOG_RESET)
    bench.sim_advance(3000)

    #Check that the device has been reset (WDR too early)
    assert MISCREG.GPIOR0 == 0x00
    assert RSTCTRL.RSTFR.WDRF
