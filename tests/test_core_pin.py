# test_core_pin.py
#
# Copyright 2023-2025 Clement Savergne <csavergne@yahoo.com>
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
from _test_utils import PinState, DictSignalHook


Floating = PinState.Floating
Analog = PinState.Analog
High = PinState.High
Low = PinState.Low
PullDown = PinState.PullDown
PullUp = PinState.PullUp
Shorted = PinState.Shorted
StateError = PinState.Error

states = [Floating, PullDown, PullUp, Analog, High, Low]

resolved_matrix = [
    [Floating, PullDown, PullUp, Analog,  High,    Low    ],
    [PullDown, PullDown, PullUp, Analog,  High,    Low    ],
    [PullUp,   PullUp,   PullUp, Analog,  High,    Low    ],
    [Analog,   Analog,   Analog, Analog,  Shorted, Shorted],
    [High,     High,     High,   Shorted, High,    Shorted],
    [Low,      Low,      Low,    Shorted, Shorted, Low    ],
]


@pytest.fixture
def wire():
    return corelib.Wire()

@pytest.fixture
def pin():
    return corelib.Pin(corelib.str_to_id('test'))


def test_wire_initial_state(wire):
    assert wire.state() == Floating
    assert wire.state().level() == 0.5
    assert not wire.digital_state()


def test_wire_voltage(wire):
    wire.set_state(Analog, 0.25)
    assert wire.voltage() == 0.25

    wire.set_state(Analog, 0.75)
    assert wire.voltage() == 0.75

    #Check bounds
    wire.set_state(Analog, 1.5)
    assert wire.voltage() == 1.0
    wire.set_state(Analog, -0.5)
    assert wire.voltage() == 0.0

    #Check forced value for digital states
    wire.set_state(Low, 0.75)
    assert wire.voltage() ==  0.0
    wire.set_state(High, 0.25)
    assert wire.voltage() == 1.0


def test_wire_resolution():
    w1 = corelib.Wire()
    w2 = corelib.Wire()
    w1.attach(w2)
    for i, s1 in enumerate(states):
        for j, s2 in enumerate(states):
            sr = resolved_matrix[i][j]
            w1.set_state(s1, 0.5)
            w2.set_state(s2, 0.5)
            assert w1.state() == sr
            assert w2.state() == sr

    w1.set_state(Analog, 0.3)
    w1.set_state(Analog, 0.5)
    assert w1.state() == Shorted
    assert w2.state() == Shorted


def test_wire_signal(wire):
    hook = DictSignalHook(wire.signal())

    StateChange = corelib.Wire.SignalId.StateChange
    VoltageChange = corelib.Wire.SignalId.VoltageChange
    DigitalChange = corelib.Wire.SignalId.DigitalChange

    wire.set_state(Low)
    assert hook.pop_data(StateChange) == Low
    assert hook.pop_data(VoltageChange) == 0.0

    wire.set_state(PullUp)
    assert hook.pop_data(StateChange) == PullUp
    assert hook.pop_data(VoltageChange) == 1.0
    assert hook.pop_data(DigitalChange) == 1

    wire.set_state(Analog, 1.0)
    assert hook.pop_data(StateChange) == Analog
    assert not hook.has_data(VoltageChange)
    assert not hook.has_data(DigitalChange)

    wire.set_state(Analog, 0.75)
    assert hook.has_data(StateChange)
    assert hook.pop_data(VoltageChange) == 0.75
    assert not hook.has_data(DigitalChange)

    wire.set_state(Analog, 0.25)
    assert hook.has_data(StateChange)
    assert hook.pop_data(VoltageChange) == 0.25
    assert hook.pop_data(DigitalChange) == 0


def test_wire_attach():
    a = corelib.Wire()
    b = corelib.Wire()

    assert not a.attached()
    assert a.siblings() == [a]
    assert not b.attached()
    assert b.siblings() == [b]

    a.attach(b)
    assert a.attached()
    assert a.attached(b)
    assert b.attached(a)
    assert set(a.siblings()) == {a, b}
    assert set(b.siblings()) == {a, b}

    c = corelib.Wire(a)
    assert set(a.siblings()) == {a, b, c}

    del b
    assert set(a.siblings()) == {a, c}


def test_wire_char_state(wire):
    wire.set_state('H')
    assert wire.state() == High

    wire.set_state('L')
    assert wire.state() == Low

    wire.set_state('U')
    assert wire.state() == PullUp

    wire.set_state('D')
    assert wire.state() == PullDown

    wire.set_state('Z')
    assert wire.state() == Floating

    wire.set_state('A')
    assert wire.state() == Analog

    wire.set_state('S')
    assert wire.state() == Shorted

    wire.set_state('B')
    assert wire.state() == StateError


def test_pin_driver():

    class TestPinDriver(corelib.PinDriver):

        def __init__(self):
            super().__init__(0x10, 2)
            self.controls = corelib.Pin.controls_t()
    
        def override_gpio(self, index, controls):
            return self.controls

    pm = corelib.PinManager([0xAA, 0xBB, 0xCC, 0xDD])
    pinAA = pm.pin(0xAA)
    pinBB = pm.pin(0xBB)
    pinCC = pm.pin(0xCC)
    pinDD = pm.pin(0xDD)

    drv = TestPinDriver()
    result = pm.register_driver(drv)
    assert result

    assert drv.pin_state(0) == StateError
    assert drv.pin_state(1) == StateError
    assert pm.current_mux(0x10) == 0
    assert pm.current_mux_pins(0x10) == [0, 0]

    result = pm.add_mux_config(0x10, [0xAA, 0xBB], 1)
    assert result
    result = pm.add_mux_config(0x10, [0xCC, 0xDD], 2)
    assert result

    assert pm.current_mux(0x10) == 1
    assert pm.current_mux_pins(0x10) == [0xAA, 0xBB]

    pinAA.set_gpio_controls(corelib.Pin.controls_t(1, 0))
    pinBB.set_gpio_controls(corelib.Pin.controls_t(1, 1))
    assert pinAA.state() == Low
    assert pinBB.state() == High
    assert drv.pin_state(0) == Low
    assert drv.pin_state(1) == High

    drv.set_enabled(True)
    assert drv.pin_state(0) == Floating
    assert drv.pin_state(1) == Floating
    assert pinAA.state() == Floating
    assert pinBB.state() == Floating

    drv.controls.pull_up = True
    drv.update_pin_state(0)
    assert pinAA.state() == PullUp

    pm.set_current_mux(0x10, 2)
    assert pm.current_mux(0x10) == 2
    assert pm.current_mux_pins(0x10) == [0xCC, 0xDD]
    assert pinAA.state() == Low
    assert pinBB.state() == High
    assert pinCC.state() == PullUp
    assert pinDD.state() == PullUp
