# test_avr_port.py
#
# Copyright 2023-2024 Clement Savergne <csavergne@yahoo.com>
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


'''
Test of the GPIO port controller on ATMega328
'''


@pytest.fixture
def bench():
    return bench_m328()


def test_avr_port_output(bench):
    port = bench.dev.PORTB
    pinB0 = bench.dev.pins['PB0']
    pinB7 = bench.dev.pins['PB7']
    port_hook = DictSignalHook(port.signal())

    assert pinB0.state() == PinState.Floating
    assert pinB7.state() == PinState.Floating

    port.DDR = 0xFF
    port.PORT = 0xAA

    assert pinB0.state() == PinState.Low
    assert pinB7.state() == PinState.High
    assert port_hook.has_data(0)
    assert port_hook.pop_data() == 0xAA
    assert port.PIN == 0xAA


def test_avr_port_input(bench):
    port = bench.dev.PORTB
    pinB0 = bench.dev.pins['PB0']
    port_hook = DictSignalHook(port.signal())

    assert pinB0.state() == PinState.Floating

    pinB0.set_external_state(PinState.Low)
    assert pinB0.state() == PinState.Low
    assert port.PIN == 0x00
    assert port_hook.pop_data() == 0x00

    pinB0.set_external_state(PinState.High)
    assert port.PIN == 0x01
    assert port_hook.pop_data() == 0x01


def test_avr_port_pullup(bench):
    port = bench.dev.PORTB
    port.DDR = 0x00
    port.PORT = 0x01
    assert bench.dev.pins['PB0'].state() == PinState.PullUp
    assert port.PIN == 0x01
