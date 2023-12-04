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

const Pin::state_t DEFAULT_STATE = { Pin::State_Floating, 0.5 };
const Pin::state_t ERROR_STATE = { Pin::State_Shorted, 0.5 };

#define STATE_DIGITAL           0x01
#define STATE_BOOL_HIGH         0x02
#define STATE_DRIVEN            0x04


/**
   Build a pin.

   \param id Identifier for the pin which should be unique
 */
Pin::Pin(pin_id_t id)
:m_id(id)
,m_ext_state(DEFAULT_STATE)
,m_gpio_state(DEFAULT_STATE)
,m_resolved_state(DEFAULT_STATE)
{
    //To ensure there is an initial persistent data stored in the signal
    m_signal.set_data(Signal_StateChange, (int) State_Floating);
    m_signal.set_data(Signal_DigitalChange, 0);
    m_signal.set_data(Signal_VoltageChange, 0.5);
}

/**
   Set the external electrical state of the pin.

   \param state new external electrical state
   \param voltage new voltage value, relative to VCC.

   \note The voltage value is used only when state is Analog,
   and contrained to the range [0.0; 1.0].
 */
void Pin::set_external_state(State state, double voltage)
{
    voltage = normalise_level(state, voltage);
    m_ext_state = { state, voltage };
    update_resolved_state();
}


/**
   Set the electrical state of the pin as controlled by the GPIO port.

   \param state new internal electrical state
 */
void Pin::set_gpio_state(State state)
{
    if (state & STATE_DIGITAL)
        m_gpio_state = { state, ((state & STATE_BOOL_HIGH) ? 1.0 : 0.0) };
    else
        m_gpio_state = DEFAULT_STATE;

    update_resolved_state();
}


/*
   returns a voltage level taking into account the state and constrained
   to the range [0.0; 1.0]
 */
double Pin::normalise_level(State state, double level)
{
    //If the state is analog, trim the voltage to the correct range
    if (state == State_Analog) {
        if (level < 0.0) level = 0.0;
        if (level > 1.0) level = 1.0;
    }
    //If the state is digital, ensure the voltage level is consistent with the
    //digital level
    else if (state & STATE_DIGITAL) {
        level = (state & STATE_BOOL_HIGH) ? 1.0 : 0.0;
    }
    //Other cases : force to the default value
    else {
        level = 0.5;
    }

    return level;
}


/**
   Resolves the electrical state from the combination of
   the internal and external states into a single state.

   \return the resolved state
 */
Pin::state_t Pin::resolved_state(const state_t& gpio, const state_t& ext)
{
    switch (gpio.state) {
        case State_Floating:
            return ext;

        case State_PullUp:
            if (ext.state & STATE_DRIVEN)
                return ext;
            else
                return gpio;

        case State_PullDown:
            //Any state other than Floating or PullDown
            if (ext.state & (STATE_DRIVEN | STATE_BOOL_HIGH))
                return ext;
            else
                return gpio;

        case State_High:
        case State_Low:
            if (gpio.state == ext.state || !(ext.state & STATE_DRIVEN))
                return gpio;
            else
                return ERROR_STATE;

        default:
            return ERROR_STATE;
    }
}


/*
   Update the resolved state and raise the corresponding signals
 */
void Pin::update_resolved_state()
{
    state_t old_state = m_resolved_state;
    bool old_dig_state = digital_state();

    m_resolved_state = resolved_state(m_gpio_state, m_ext_state);

    if (m_resolved_state.state != old_state.state)
        m_signal.raise(Signal_StateChange, (int) m_resolved_state.state);

    if (m_resolved_state.level != old_state.level)
        m_signal.raise(Signal_VoltageChange, m_resolved_state.level);

    bool dig_state = digital_state();
    if (dig_state != old_dig_state)
        m_signal.raise(Signal_DigitalChange, (unsigned char) dig_state);
}


/**
   Returns the electrical state expressed as a boolean state.
   If the state is digital, it returns true if the state is high or
   false if it's low.
   In other states, true is returned if the voltage is greater than 0.5.

   \return the resolved state
 */
bool Pin::digital_state() const
{
    if (m_resolved_state.state & STATE_DIGITAL)
        return (m_resolved_state.state & STATE_BOOL_HIGH);
    else
        return (m_resolved_state.level > 0.5);
}


/**
   Callback override for receiving signal changes
 */
void Pin::raised(const signal_data_t& sigdata, int)
{
    if (sigdata.sigid == Signal_StateChange)
        set_external_state((State) sigdata.data.as_int());

    else if (sigdata.sigid == Signal_VoltageChange)
        if (m_ext_state.state == State_Analog)
            set_external_state(State_Analog, sigdata.data.as_double());
}
