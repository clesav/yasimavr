/*
 * pin.sip
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

    This file is part of yasim-avr.

    yasim-avr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    yasim-avr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.
 */

//=======================================================================================

typedef sim_id_t pin_id_t;

class Pin : public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_pin.h"
%End

public:

    enum State /BaseType=IntEnum/ {
        State_Floating  /PyName=Floating/,
        State_PullUp    /PyName=PullUp/,
        State_PullDown  /PyName=PullDown/,
        State_Analog    /PyName=Analog/,
        State_High      /PyName=High/,
        State_Low       /PyName=Low/,
        State_Shorted   /PyName=Shorted/,
    };

    enum SignalId /BaseType=IntEnum/ {
        Signal_StateChange          /PyName=StateChange/,
        Signal_DigitalChange        /PyName=DigitalChange/,
        Signal_VoltageChange        /PyName=VoltageChange/,
    };

    struct state_t {
        Pin::State state;
        double level;
    };

    static const char* StateName(Pin::State);

    Pin(pin_id_t);

    void set_external_state(Pin::State, double = 0.0);
    void set_gpio_state(Pin::State);
    pin_id_t id() const;
    Pin::State state() const;
    bool digital_state() const;
    double voltage() const;

    DataSignal& signal();

    virtual void raised(const signal_data_t&, int);

};
