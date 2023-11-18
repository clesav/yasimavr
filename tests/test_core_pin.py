# test_core_pin.py
#
# Copyright 2023 Clement Savergne <csavergne@yahoo.com>
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
import yasimavr.lib.core as corelib
from _test_utils import DictSignalHook


Floating = corelib.Pin.State.Floating
Analog = corelib.Pin.State.Analog
High = corelib.Pin.State.High
Low = corelib.Pin.State.Low


@pytest.fixture
def pin():
    return corelib.Pin(corelib.str_to_id('test'))


def test_pin_initial_state(pin):
    assert pin.id() == corelib.str_to_id('test')
    assert pin.state() == Floating
    assert pin.analog_value() == 0.0
    assert pin.digital_state() == Low


def test_pin_external_voltage(pin):
    pin.set_external_state(Analog)
    pin.set_external_analog_value(0.25)
    assert pin.analog_value() == 0.25

    pin.set_external_analog_value(0.75)
    assert pin.analog_value() == 0.75

    #Check bounds
    pin.set_external_analog_value(1.5)
    assert pin.analog_value() == 1.0
    pin.set_external_analog_value(-0.5)
    assert pin.analog_value() == 0.0

    #Check forced value for digital states
    pin.set_external_state(Low)
    pin.set_external_analog_value(0.75)
    assert pin.analog_value() ==  0.0
    pin.set_external_state(High)
    pin.set_external_analog_value(0.25)
    assert pin.analog_value() == 1.0
