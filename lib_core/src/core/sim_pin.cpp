/*
 * sim_pin.cpp
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

#include "sim_pin.h"

YASIMAVR_USING_NAMESPACE


/**
   Map the state to its name, for debug and logging purpose.

   \param state

   \return name of the state
 */
const char* Pin::StateName(State state)
{
    switch(state) {
        case State_Floating: return "Floating";
        case State_PullUp: return "Pull up";
        case State_PullDown: return "Pull down";
        case State_Analog: return "Analog";
        case State_High: return "High";
        case State_Low: return "Low";
        case State_Shorted: return "Shorted";
        default: return "";
    };
}

/**
   Build a pin.

   \param id Identifier for the pin which should be unique
 */
Pin::Pin(pin_id_t id)
:m_id(id)
,m_ext_state(State_Floating)
,m_int_state(State_Floating)
,m_resolved_state(State_Floating)
,m_analog_value(0.0)
{
    //To ensure there is an initial persistent data stored in the signal
    m_signal.raise(Signal_DigitalStateChange, State_Floating);
    m_signal.raise(Signal_AnalogValueChange, 0.0);
}

/**
   Set the external electrical state of the pin.

   \param state new external electrical state
 */
void Pin::set_external_state(State state)
{
    State prev_digstate = digital_state();
    m_ext_state = state;
    m_resolved_state = resolve_state();
    State digstate = digital_state();
    if (digstate != prev_digstate) {
        m_signal.raise(Signal_DigitalStateChange, digstate);
        m_signal.raise(Signal_AnalogValueChange, analog_value());
    }
}

/**
   Set the internal electrical state of the pin.
   This is only used by general purpose port models.

   \param state new internal electrical state
 */
void Pin::set_internal_state(State state)
{
    State prev_digstate = digital_state();
    m_int_state = state;
    m_resolved_state = resolve_state();
    State digstate = digital_state();
    if (digstate != prev_digstate) {
        m_signal.raise(Signal_DigitalStateChange, digstate);
        m_signal.raise(Signal_AnalogValueChange, analog_value());
    }
}

/**
   Resolves the electrical state from the combination of
   the internal and external states into a single state.

   \return the resolved state
 */
Pin::State Pin::resolve_state()
{
    switch (m_int_state) {
        case State_Floating:
            return m_ext_state;

        case State_PullUp:
            switch (m_ext_state) {
                case State_Floating: return State_PullUp;
                case State_PullDown: return State_Floating;
                default: return m_ext_state;
            }

        case State_PullDown:
            switch(m_ext_state) {
                case State_Floating: return State_PullDown;
                case State_PullUp: return State_Floating;
                default: return m_ext_state;
            }

        case State_Analog:
            if (m_ext_state >= State_Analog)
                return State_Shorted;
            else
                return State_Analog;

        case State_High:
            if (m_ext_state >= State_Analog && m_ext_state != State_High)
                return State_Shorted;
            else
                return State_High;

        case State_Low:
            if (m_ext_state >= State_Analog && m_ext_state != State_Low)
                return State_Shorted;
            else
                return State_Low;

        default: return State_Shorted;
    }
}

/**
   Set the external analog voltage.
   This has no effect in the external state is not Analog.

   \param v the analog voltage value in the range [0.0; 1.0]
 */
void Pin::set_external_analog_value(double v)
{
    //If the pin state is not Analog, ignore any change
    if (m_ext_state != State_Analog)
        return;

    //Trim the value to the valid interval [0, 1]
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    //Backup the current digital state to detect a potential change
    State prev_digstate = digital_state();

    m_analog_value = v;

    //If the digital state has change, raise the digital signal
    State digstate = digital_state();
    if (digstate != prev_digstate)
        m_signal.raise(Signal_DigitalStateChange, digstate);

    //Raise the analog signal in any case
    m_signal.raise(Signal_AnalogValueChange, analog_value());
}

/**
   Compute and return the analog level corresponding to the
   resolved electrical state.

   \return the resolved analog value in the range [0.0; 1.0]
 */
double Pin::analog_value() const
{
    switch (m_resolved_state) {
        case State_Floating:
        case State_Analog:
            return m_analog_value;

        case State_PullUp:
        case State_High:
            return 1.0;

        case State_PullDown:
        case State_Low:
            return 0.0;

        default:
            return 0.5;
    }
}

/**
   \return the resolved electrical state. Can be one of
   High, Low or Shorted.
 */
Pin::State Pin::digital_state() const
{
    switch (m_resolved_state) {
        case State_PullUp:
        case State_High:
            return State_High;

        case State_PullDown:
        case State_Low:
            return State_Low;

        case State_Shorted:
            return State_Shorted;

        default:
            return m_analog_value > 0.5 ? State_High : State_Low;
    }
}

/**
   Callback override for receiving signal changes
 */
void Pin::raised(const signal_data_t& sigdata, int)
{
    if (sigdata.sigid == Signal_DigitalStateChange)
        set_external_state((State) sigdata.data.as_int());
    else if (sigdata.sigid == Signal_AnalogValueChange)
        set_external_analog_value(sigdata.data.as_double());
}
