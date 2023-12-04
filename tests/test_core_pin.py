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
PullDown = corelib.Pin.State.PullDown
PullUp = corelib.Pin.State.PullUp
Shorted = corelib.Pin.State.Shorted

states = [Floating, PullDown, PullUp, Analog, High, Low]

resolved_matrix = [
    [Floating, PullDown, PullUp, Analog,  High,    Low    ],
    [PullDown, PullDown, PullUp, Analog,  High,    Low    ],
    [PullUp,   PullUp,   PullUp, Analog,  High,    Low    ],
    [Floating, PullDown, PullUp, Analog,  High,    Low    ],
    [High,     High,     High,   Shorted, High,    Shorted],
    [Low,      Low,      Low,    Shorted, Shorted, Low    ],
]


@pytest.fixture
def pin():
    return corelib.Pin(corelib.str_to_id('test'))


def test_pin_initial_state(pin):
    assert pin.id() == corelib.str_to_id('test')
    assert pin.state() == Floating
    assert pin.voltage() == 0.5
    assert not pin.digital_state()


def test_pin_external_voltage(pin):
    pin.set_external_state(Analog, 0.25)
    assert pin.voltage() == 0.25

    pin.set_external_state(Analog, 0.75)
    assert pin.voltage() == 0.75

    #Check bounds
    pin.set_external_state(Analog, 1.5)
    assert pin.voltage() == 1.0
    pin.set_external_state(Analog, -0.5)
    assert pin.voltage() == 0.0

    #Check forced value for digital states
    pin.set_external_state(Low, 0.75)
    assert pin.voltage() ==  0.0
    pin.set_external_state(High, 0.25)
    assert pin.voltage() == 1.0


def test_pin_resolution(pin):
    for i, s1 in enumerate(states):
        for j, s2 in enumerate(states):
            sr = resolved_matrix[i][j]
            pin.set_gpio_state(s1)
            pin.set_external_state(s2)
            assert pin.state() == sr


def test_pin_signal(pin):
    hook = DictSignalHook(pin.signal())

    StateChange = corelib.Pin.SignalId.StateChange
    VoltageChange = corelib.Pin.SignalId.VoltageChange
    DigitalChange = corelib.Pin.SignalId.DigitalChange

    pin.set_external_state(Low)
    assert hook.has_data(StateChange)
    assert hook.pop(StateChange)[0].data == Low
    assert hook.has_data(VoltageChange)
    assert hook.pop(VoltageChange)[0].data == 0.0

    pin.set_external_state(PullUp)
    assert hook.has_data(StateChange)
    assert hook.pop(StateChange)[0].data == PullUp
    assert hook.has_data(VoltageChange)
    assert hook.pop(VoltageChange)[0].data == 1.0
    assert hook.has_data(DigitalChange)
    assert hook.pop(DigitalChange)[0].data == 1

    pin.set_external_state(Analog, 1.0)
    assert hook.has_data(StateChange)
    assert hook.pop(StateChange)[0].data == Analog
    assert not hook.has_data(VoltageChange)
    assert not hook.has_data(DigitalChange)

    pin.set_external_state(Analog, 0.75)
    assert not hook.has_data(StateChange)
    assert hook.has_data(VoltageChange)
    assert hook.pop(VoltageChange)[0].data == 0.75
    assert not hook.has_data(DigitalChange)

    pin.set_external_state(Analog, 0.25)
    assert not hook.has_data(StateChange)
    assert hook.has_data(VoltageChange)
    assert hook.pop(VoltageChange)[0].data == 0.25
    assert hook.has_data(DigitalChange)
    assert hook.pop(DigitalChange)[0].data == 0
