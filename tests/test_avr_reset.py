# test_avr_reset.py
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
from yasimavr.lib.core import Device


'''
Test of the reset controller on ATMega328
'''


@pytest.fixture
def bench():
    return bench_m328()


def test_avr_reset_flags(bench):
    MCUSR = bench.dev.RSTCTRL.MCUSR

    assert MCUSR.PORF
    MCUSR.PORF = 0
    assert not MCUSR.PORF
    MCUSR.PORF = 1
    assert not MCUSR.PORF

    bench.dev_model.reset(Device.ResetFlag.BOD)
    assert MCUSR.BORF

    bench.dev_model.reset(Device.ResetFlag.WDT)
    assert MCUSR.BORF
    assert MCUSR.WDRF
    MCUSR.BORF = 0
    assert not MCUSR.BORF
    assert MCUSR.WDRF

    bench.dev_model.reset(Device.ResetFlag.PowerOn)
    assert MCUSR.PORF
    assert not MCUSR.WDRF
