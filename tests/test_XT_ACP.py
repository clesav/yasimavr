# test_XT_ACP.py
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
Automatic tests for the ACP simulation model (XT architecture)
'''


import pytest
from _test_bench_xt import BenchXT, TESTFW_M4809
from _test_utils import PinState
import yasimavr.lib.core as corelib
from yasimavr.device_library import load_device


@pytest.fixture
def bench():
    dev_model = load_device('atmega4809')
    b = BenchXT(dev_model, TESTFW_M4809)
    b.pin_pos = b.dev.pins['PD2']
    b.pin_neg = b.dev.pins['PD3']

    return b


def test_xt_acp_state(bench):
    ACP = bench.dev.ACP0

    ACP.CTRLA.ENABLE = 1
    ACP.MUXCTRLA.MUXPOS = 0
    ACP.MUXCTRLA.MUXNEG = 0

    bench.pin_pos.set_external_state('A', 0.0)
    bench.pin_neg.set_external_state('A', 0.5)
    assert not ACP.STATUS.STATE

    bench.pin_pos.set_external_state('A', 1.0)
    assert ACP.STATUS.STATE

    bench.pin_pos.set_external_state('A', 0.0)
    assert not ACP.STATUS.STATE


def test_xt_acp_dacref(bench):
    ACP = bench.dev.ACP0

    bench.dev.VREF.CTRLA.AC0REFSEL = 0x07
    ACP.CTRLA.ENABLE = 1
    ACP.MUXCTRLA.MUXPOS = 0
    ACP.MUXCTRLA.MUXNEG = 3

    bench.pin_pos.set_external_state('A', 0.2)
    assert ACP.STATUS.STATE

    ACP.DACREF = 64
    assert not ACP.STATUS.STATE

    bench.pin_pos.set_external_state('A', 0.3)
    assert ACP.STATUS.STATE

    bench.pin_pos.set_external_state('A', 0.2)
    assert not ACP.STATUS.STATE

    ACP.DACREF = 192
    bench.pin_pos.set_external_state('A', 0.7)
    assert not ACP.STATUS.STATE

    bench.pin_pos.set_external_state('A', 0.8)
    assert ACP.STATUS.STATE


def test_xt_acp_cmp(bench):
    ACP = bench.dev.ACP0

    ACP.CTRLA.ENABLE = 1
    ACP.MUXCTRLA.MUXPOS = 0
    ACP.MUXCTRLA.MUXNEG = 0
    bench.pin_pos.set_external_state('A', 0.0)
    bench.pin_neg.set_external_state('A', 0.5)
    assert not ACP.STATUS.CMP

    bench.pin_pos.set_external_state('A', 1.0)
    assert ACP.STATUS.CMP

    bench.pin_pos.set_external_state('A', 0.0)
    assert ACP.STATUS.CMP

    ACP.STATUS.CMP = 1
    assert not ACP.STATUS.CMP
