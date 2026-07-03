# test_avr_clock.py
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


'''
Automatic tests for the clock controller simulation model (AVR architecture)
'''


import pytest
from _test_bench_avr import bench_m328, PATH_TESTFW_M328
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_avr as archlib
from yasimavr.device_library import load_device
from yasimavr.device_library.descriptors import convert_to_regbit


@pytest.fixture
def bench():
    fw = corelib.Firmware.read_elf(PATH_TESTFW_M328)
    return bench_m328(fw=fw, advance=False)


bs_clkdiv8 = corelib.bitspec_t(7)
bs_clksel = corelib.bitspec_t(0, 3)


def test_avr_clk_init(bench):
    CLK = bench.dev.CLKCTRL
    cm = bench.loop.cycle_manager()
    fuse_nvm = bench.dev.nvms['Fuses']

    fuse_nvm.write(bs_clkdiv8.clear_from(fuse_nvm[0]), 0)
    bench.dev_model.reset()
    assert cm.reference_frequency() == 1e6

    fuse_nvm.write(bs_clkdiv8.set_from(fuse_nvm[0]), 0)
    bench.dev_model.reset()
    assert cm.reference_frequency() == 8e6

'''
def test_xt_clk_prescaler(bench):
    CLK = bench.dev.CLKCTRL

    assert CLK.MCLKCTRLB.PEN == 1
    assert CLK.MCLKCTRLB.PDIV == 'DIV6'
    assert bench.cycler.reference_frequency() == 20e6 / 6

    CLK.MCLKCTRLB.PDIV = 0
    assert bench.cycler.reference_frequency() == 20e6 / 2

    CLK.MCLKCTRLB.PDIV = 0xC
    assert bench.cycler.reference_frequency() == 20e6 / 48

    CLK.MCLKCTRLB.PEN = 0
    assert bench.cycler.reference_frequency() == 20e6


def test_xt_clk_extclk(bench):
    CLK = bench.dev.CLKCTRL

    assert not CLK.MCLKSTATUS.SOSC

    CLK.MCLKCTRLB.PEN = 0
    CLK.MCLKCTRLA.CLKSEL = 'EXTCLK'

    assert bench.cycler.reference_frequency() == 20e6
    assert not CLK.MCLKSTATUS.EXTS
    assert CLK.MCLKSTATUS.SOSC

    reqdata = corelib.ctlreq_data_t()
    reqdata.index = archlib.ArchXT_Device.ClockSource.Ext
    reqdata.data = corelib.vardata_t(1e6)
    bench.dev_model.ctlreq('CLKCTRL', corelib.CTLREQ_CLK_SET_EXTCLK, reqdata)

    assert CLK.MCLKSTATUS.EXTS
    assert not CLK.MCLKSTATUS.SOSC
    assert bench.cycler.reference_frequency() == 1e6

    reqdata.data = corelib.vardata_t(5e4)
    bench.dev_model.ctlreq('CLKCTRL', corelib.CTLREQ_CLK_SET_EXTCLK, reqdata)
    assert bench.cycler.reference_frequency() == 5e4

    CLK.MCLKCTRLA.CLKSEL = 'OSC20M'
    assert CLK.MCLKCTRLA.CLKSEL == 'EXTCLK'

    reqdata.data = corelib.vardata_t(0)
    bench.dev_model.ctlreq('CLKCTRL', corelib.CTLREQ_CLK_SET_EXTCLK, reqdata)
    assert bench.cycler.reference_frequency() == 0.0
    assert bench.dev_model.state() == corelib.Device.State.Crashed
'''