/*
 * sim_wire.h
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

#ifndef __YASIMAVR_WIRE_H__
#define __YASIMAVR_WIRE_H__

#include "sim_types.h"
#include "sim_signal.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief General Purpose wire model.

   This class models a logical line used to represents digital/analog electrical signals.
   Its main purpose is to serve as base class for the MCU pin models and can be used
   by external component models to simulate digital or analog signals connected to a MCU model.

   Wires need to be *attached* to each other. An attachment represents an electrical connection
   between two input and/or output circuits.
   Wires have a individual state (one of the State enum values) representing how the local circuit
   is driving them (or not driving) and a resolved state, shared by all attached Wires
   representing the common state.

   For example, if Wires A and B are attached, A's state is Floating, B's state is High, the
   common resolved state is High.

   As for the rest of the library, analog voltage levels are relative to VCC, hence limited to the range [0.0, 1.0].

   \sa Pin
 */
class AVR_CORE_PUBLIC_API Wire {

public:

    /**
       Pin state enum.
       All the possible logical/analog electrical states that the wire can take.
     */
    enum StateEnum {
        //The enum values are partially a bitset (using StateFlag values) :
        //bit 0 indicates that it is a stable digital state if set
        //bit 1 is the boolean value (only if bit 0 is set too)
        //bit 2 indicates a 'driver' state if set or 'weak' if clear
        //'Weak' states
        State_Floating  = 0x00,
        State_PullUp    = 0x03,
        State_PullDown  = 0x01,
        //'Driver' states
        State_Analog    = 0x04,
        State_High      = 0x07,
        State_Low       = 0x05,
        //Special states
        State_Shorted   = 0x80,
        State_Error     = 0x90
    };


    class AVR_CORE_PUBLIC_API state_t {

    public:

        inline state_t() : m_value(State_Floating), m_level(0.5) {}
        constexpr state_t(StateEnum s, double v = 0.0) : m_value(s), m_level(normalised_level(s, v)) {}
        state_t(const state_t& other) = default;

        inline bool is_digital() const { return m_value & 0x01; }
        inline bool is_driven() const { return m_value & 0x04; }

        inline bool digital_value() const { return (m_value & 0x01) ? (m_value & 0x02) : (m_level > 0.5); }

        std::string to_string() const;

        inline bool operator==(StateEnum s) const { return m_value == s; }
        inline bool operator!=(StateEnum s) const { return m_value != s; }
        inline bool operator==(const state_t& s) const
        {
            return (s.m_value == m_value) && ((m_value != State_Analog) || (s.m_level == m_level));
        }
        inline bool operator!=(const state_t& s) const { return !(s == *this); }

        inline StateEnum value() const { return m_value; }
        inline double level() const { return m_level; }

        state_t& operator=(const state_t& other) = default;

    private:

        StateEnum m_value;
        double m_level;

        constexpr static double normalised_level(StateEnum s, double v)
        {
            //If the state is analog, trim the voltage to the correct range
            if (s == State_Analog) {
                if (v < 0.0) return 0.0;
                if (v > 1.0) return 1.0;
                else return v;
            }
            //If the state is digital, ensure the voltage level is consistent with the
            //digital level
            else if (s & 0x01) {
                return (s & 0x02) ? 1.0 : 0.0;
            }
            //Other cases : force to the default value
            else {
                return 0.5;
            }
        }

    };

    /**
       Converts an ASCII charcode into a wire state enum value :
        - 'Z'/'z' : Floating
        - 'H'/'h' : High
        - 'L'/'l' : Low
        - 'U'/'u' : PullUp
        - 'D'/'d' : PullDown
        - 'A'/'a' : Analog
        - 'S'/'s' : Shorted
        - any other value : Error
     */
    static StateEnum char2state(char c);


    /**
       Signal IDs raised by the pin.
       For all signals, the index is set to the pin ID.
     */
    enum SignalId {
        /**
          Signal raised for any change of the resolved state.
          data is set to the new state (one of State enum values)
         */
        Signal_StateChange = 0,

        /**
          Signal raised for any change of the resolved digital state.
          data is set to the new digital state (0 or 1).
         */
        Signal_DigitalChange,

        /**
           Signal raised for any change to the analog value, including
           when induced by a digital state change.
           data is set to the analog value (double, in range [0;1])
         */
        Signal_VoltageChange,
    };


    Wire();
    Wire(Wire& other);
    Wire(const Wire& other);
    virtual ~Wire();

    DataSignal& signal();

    const state_t& state() const;
    bool digital_state() const;
    double voltage() const;

    void set_state(const state_t& state);
    void set_state(StateEnum state, double v = 0.0);
    void set_state(char state, double v = 0.0);

    void attach(Wire& other);
    void detach();
    bool attached(const Wire& other) const;
    bool attached() const;
    std::vector<Wire*> siblings() const;

    Wire& operator=(Wire& other);
    Wire& operator=(const Wire& other);

    static state_t resolve_two_states(const state_t& a, const state_t& b);

protected:

    void auto_resolve_state();

    virtual state_t state_for_resolution() const;

private:

    state_t m_state;
    state_t m_resolved_state;
    Wire* m_primary;
    std::vector<Wire*> m_secondaries;
    DataSignal m_signal;

    void resolve_state();
    void set_resolved_state(const state_t& s);
    void notify_resolved_state();

};

/**
   \return the resolved state
 */
inline const Wire::state_t& Wire::state() const
{
    return m_resolved_state;
};

/**
   \return the pin voltage value
 */
inline double Wire::voltage() const
{
    return m_resolved_state.level();
}

/**
   Set the state of the wire using one of the StateEnum values
   \param state new Wire state
   \param level voltage level (only for the Analog state)
 */
inline void Wire::set_state(StateEnum state, double level)
{
    set_state(state_t(state, level));
}

/**
   Set the state of the wire using a charcode
   \sa char2state
   \param state new Wire state
   \param level voltage level (only for the Analog state)
 */
inline void Wire::set_state(char state, double level)
{
    set_state(state_t(char2state(state), level));
}

/**
   \return the signal raising the state/value changes
 */
inline DataSignal& Wire::signal()
{
    return m_signal;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_WIRE_H__
