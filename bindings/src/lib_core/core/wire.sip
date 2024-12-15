/*
 * wire.sip
 *
 *  Copyright 2024 Clement Savergne <csavergne@yahoo.com>

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

class Wire {
%TypeHeaderCode
#include "core/sim_wire.h"
%End

public:

    enum StateEnum /BaseType=IntEnum/ {
        State_Floating  /PyName=Floating/,
        State_PullUp    /PyName=PullUp/,
        State_PullDown  /PyName=PullDown/,
        State_Analog    /PyName=Analog/,
        State_High      /PyName=High/,
        State_Low       /PyName=Low/,
        State_Shorted   /PyName=Shorted/,
        State_Error     /PyName=Error/,
    };

    class state_t {
    public:
        state_t();
        state_t(Wire::StateEnum, double = 0.0);
        state_t(const state_t&);

        bool is_digital() const;
        bool is_driven() const;
        bool digital_value() const;

        const char* __repr__() const;
        %MethodCode
            std::string s = sipCpp->to_string();
            sipRes = s.c_str();
        %End

        int __eq__(Wire::StateEnum) const;
        %MethodCode
            sipRes = (int)(*sipCpp == a0);
        %End

        int __eq__(const state_t&) const;
        %MethodCode
            sipRes = (int)(*sipCpp == *a0);
        %End

        int __ne__(Wire::StateEnum) const;
        %MethodCode
            sipRes = (int)(*sipCpp != a0);
        %End

        int __ne__(const state_t&) const;
        %MethodCode
            sipRes = (int)(*sipCpp != *a0);
        %End

        Wire::StateEnum value() const;
        double level() const;
    };

    static Wire::StateEnum char2state(char);


    enum SignalId /BaseType=IntEnum/ {
        Signal_StateChange          /PyName=StateChange/,
        Signal_DigitalChange        /PyName=DigitalChange/,
        Signal_VoltageChange        /PyName=VoltageChange/,
    };


    Wire();
    Wire(Wire&);

    DataSignal& signal();

    Wire::state_t state() const;
    bool digital_state() const;
    double voltage() const;

    void set_state(const Wire::state_t&);
    void set_state(Wire::StateEnum, double = 0.0);
    void set_state(char, double = 0.0);

    void attach(Wire&);
    void detach();
    bool attached(const Wire&) const;
    bool attached() const;
    std::vector<Wire*> siblings() const;

    static Wire::state_t resolve_two_states(const Wire::state_t&, const Wire::state_t&);

};
