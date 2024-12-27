# test_xt_port.py
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
import yasimavr.lib.arch_xt as archlib
from _test_bench_xt import BenchXT, TESTFW_M4809
from _test_utils import PinState, DictSignalHook


@pytest.fixture
def bench():
    return BenchXT('atmega4809', TESTFW_M4809)


def test_xt_port_output(bench):
    porta = bench.dev.PORTA
    pinA0 = bench.dev.pins['PA0']
    pinA7 = bench.dev.pins['PA7']

    print([bench.dev.pins['PA' + str(i)].state() for i in range(8)])
    assert pinA0.state() == PinState.Floating
    assert pinA7.state() == PinState.Floating

    porta.DIR = 0xFF
    porta.OUT = 0xAA

    print([bench.dev.pins['PA' + str(i)].state() for i in range(8)])
    assert pinA0.state() == PinState.Low
    assert pinA7.state() == PinState.High
    #Set register test
    porta.OUTSET = 0x01
    assert pinA0.state() == PinState.High
    #Clear register test
    porta.OUTCLR = 0x01
    assert pinA0.state() == PinState.Low
    #Toggle register test
    porta.OUTTGL = 0x01
    assert pinA0.state() == PinState.High
    porta.OUTTGL = 0x01
    assert pinA0.state() == PinState.Low

    porta.DIR = 0x00
    porta.OUT = 0x00
    assert pinA0.state() == PinState.Floating
    porta.PIN0CTRL.PULLUPEN = 1
    assert pinA0.state() == PinState.PullUp


def test_xt_port_input(bench):
    porta = bench.dev.PORTA
    pinA0 = bench.dev.pins['PA0']
    hook = DictSignalHook(porta.signal())

    assert pinA0.state() == PinState.Floating
    pinA0.set_external_state('L')
    assert pinA0.state() == PinState.Low
    assert porta.IN == 0x00
    
    pinA0.set_external_state('H')
    assert porta.IN == 0x01

    pinA0.set_external_state('L')
    porta.PIN0CTRL.PULLUPEN = 1
    assert porta.IN == 0x00

    pinA0.set_external_state('Z')
    assert pinA0.state() == PinState.PullUp
    assert porta.IN == 0x01
    assert hook.has_data()
    sigdata = hook.pop()[0]
    print(hex(sigdata.data.value()))
    assert sigdata.data == 0x01


def test_xt_port_irq(bench):
    porta = bench.dev.PORTA
    pinA0 = bench.dev.pins['PA0']
    pinA1 = bench.dev.pins['PA1']

    pinA0.set_external_state('L')
    porta.PIN0CTRL.ISC = 'RISING'
    pinA0.set_external_state('H')
    bench.sim_advance(100)
    assert bench.dev.MISC.GPIOR0 == 6
    assert bench.dev.MISC.GPIOR1 == 0x01

    bench.dev.MISC.GPIOR0 = 0
    pinA0.set_external_state('L')
    bench.sim_advance(100)
    assert bench.dev.MISC.GPIOR0 == 0

    porta.PIN0CTRL.ISC = 'INTDISABLE'
    porta.PIN1CTRL.ISC = 'FALLING'
    pinA1.set_external_state('H')
    bench.sim_advance(100)
    assert bench.dev.MISC.GPIOR0 == 0

    pinA1.set_external_state('L')
    bench.sim_advance(100)
    assert bench.dev.MISC.GPIOR0 == 6
    assert bench.dev.MISC.GPIOR1 == 0x02
