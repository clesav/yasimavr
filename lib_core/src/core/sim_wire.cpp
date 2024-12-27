/*
 * sim_wire.cpp
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

#include "sim_wire.h"

YASIMAVR_USING_NAMESPACE


/*
 * The Wire class has concepts of 'primary' and secondary' Wire objects when they're attached.
 * These concepts only have an internal use, from an external point of view there is no
 * functional difference between a primary wire and a secondary wire.
 * The responsibilities of the primary wire are to manage the record of all secondaries and to
 * resolve the common state between all attached wires -including itself- during an update.
 * To achieve these goals, it keeps a vector of pointers to the secondary wires and a pointer to
 * primary hence the rule : "A Wire object is primary if and only if its m_primary attribute is nullptr".
 * Note : a standalone (i.e. not attached to anything) Wire object is always primary.
 */


//=======================================================================================

/**
   Returns a string representation of the wire state
 */
std::string Wire::state_t::to_string() const
{
    switch(m_value) {
        case State_Floating: return "Floating";
        case State_PullUp: return "Pull up";
        case State_PullDown: return "Pull down";
        case State_Analog: {
            char s[50];
            sprintf(s, "Analog [%f]", m_level);
            return s;
        } break;
        case State_High: return "High";
        case State_Low: return "Low";
        case State_Shorted: return "Shorted";
        default: return "Error";
    };
}


Wire::StateEnum Wire::char2state(char c)
{
    switch (c) {
        case 'Z':
        case 'z':
            return State_Floating;

        case 'H':
        case 'h':
            return State_High;

        case 'L':
        case 'l':
            return State_Low;

        case 'U':
        case 'u':
            return State_PullUp;

        case 'D':
        case 'd':
            return State_PullDown;

        case 'A':
        case 'a':
            return State_Analog;

        case 'S':
        case 's':
            return State_Shorted;

        default:
            return State_Error;
    }
}


/**
   Build a Wire.
 */
Wire::Wire()
:m_state()
,m_resolved_state()
,m_primary(nullptr)
{
    //To ensure there is an initial persistent data stored in the signal
    m_signal.set_data(Signal_StateChange, (int) State_Floating);
    m_signal.set_data(Signal_DigitalChange, 0);
    m_signal.set_data(Signal_VoltageChange, 0.5);
}

/**
   Build a Wire by copy and attach to another Wire.
 */
Wire::Wire(Wire& other)
:Wire()
{
    *this = other;
}

/**
   Build a Wire by copy. The new instance is not attached to the other Wire.
 */
Wire::Wire(const Wire& other)
:Wire()
{
    *this = other;
}


Wire::~Wire()
{
    detach();
}

/**
   \return whether 'this' is attached to the other Wire object.
 */
bool Wire::attached(const Wire& other) const
{
    if (!m_primary)
        return other.m_primary == this;
    else if (!other.m_primary)
        return m_primary == &other;
    else
        return m_primary == other.m_primary;
}

/**
   \return whether 'this' is attached to any other Wire object.
 */
bool Wire::attached() const
{
    return m_primary || m_secondaries.size();
}

/**
   Return all Wire objects 'this' is attached to.
   The returned vector also contains 'this'.

   If a and b are Wire objects attached to each other, "a.siblings()" and "b.siblings()" will return
   the same result, albeit possibly in a different order.
 */
std::vector<Wire*> Wire::siblings() const
{
    if (m_primary)
        return m_primary->siblings();

    std::vector<Wire*> v = m_secondaries;
    v.push_back(const_cast<Wire*>(this));
    return v;
}

/**
   Attach another Wire.

   \note if a and b are standalone Wire objects (i.e. not already attached), "a.attach(b)" and "b.attach(a)"
   have the same functional result.
   If a or b are already attached to other Wire objects, the effect is essentially to merge
   both groups of Wire objects.
 */
void Wire::attach(Wire& other)
{
    //If this and other are already attached, nothing to do.
    if (attached(other))
        return;

    if (m_primary) {
        m_primary->attach(other);
        return;
    }

    if (other.m_primary) {
        attach(*other.m_primary);
        return;
    }

    //From this point onwards, both 'this' and 'other' are the primaries we want to attach together.

    //Make 'other' a secondary of 'this'
    other.m_primary = this;
    m_secondaries.push_back(&other);

    //Transfer all secondaries of 'other' to 'this'
    for (auto s : other.m_secondaries) {
        s->m_primary = this;
        m_secondaries.push_back(s);
    }
    other.m_secondaries.clear();

    auto_resolve_state();
}

/**
   Detach from all Wire 'this' is attached to.
 */
void Wire::detach()
{
    if (m_primary) {
        for (auto it = m_primary->m_secondaries.begin(); it != m_primary->m_secondaries.end(); ++it) {
            if (*it == this) {
                m_primary->m_secondaries.erase(it);
                break;
            }
        }

        m_primary->auto_resolve_state();
        m_primary = nullptr;
    }

    else if (m_secondaries.size()) {
        Wire* new_primary = m_secondaries[0];
        m_secondaries.erase(m_secondaries.begin());

        new_primary->m_primary = nullptr;

        for (auto sec : m_secondaries)
            sec->m_primary = new_primary;
        new_primary->m_secondaries = std::move(m_secondaries);

        new_primary->auto_resolve_state();
    }

    auto_resolve_state();
}

/**
   Set the state of the Wire.
 */
void Wire::set_state(const state_t& state)
{
    m_state = state;
    auto_resolve_state();
}

/**
   Resolution algorithm for combining two state_t structures representing the logical state
   of two Wires attached together and returning the common resolved state.
 */
Wire::state_t Wire::resolve_two_states(const state_t& a, const state_t& b)
{
    switch (a.value()) {
        case State_Floating:
            return b;

        case State_PullUp:
            if (b.is_driven())
                return b;
            else
                return a;

        case State_PullDown:
            //Any state other than Floating or PullDown
            if (b.value() & (0x02 | 0x04))
                return b;
            else
                return a;

        case State_High:
        case State_Low:
            if (a.value() == b.value() || !b.is_driven())
                return a;
            else
                return State_Shorted;

        case State_Analog:
            if (!b.is_driven() || (b.value() == State_Analog && a.level() == b.level()))
                return a;
            else
                return State_Shorted;

        default:
            return State_Shorted;
    }
}


void Wire::auto_resolve_state()
{
    //If 'this' is secondary, defer the calculation to the primary
    if (m_primary) {
        m_primary->auto_resolve_state();
        return;
    }

    resolve_state();
}


Wire::state_t Wire::state_for_resolution() const
{
    return m_state;
}


void Wire::resolve_state()
{
    //Determine the new common state
    state_t s = state_for_resolution();
    for (auto p : m_secondaries)
        s = resolve_two_states(p->state_for_resolution(), s);

    //Set it in all the attached pins, including this
    for (auto p : m_secondaries)
        p->set_resolved_state(s);

    set_resolved_state(s);
}


void Wire::set_resolved_state(const state_t& s)
{
    state_t old_resolved_state = m_resolved_state;
    m_resolved_state = s;

    if (s != old_resolved_state)
        m_signal.raise(Signal_StateChange, (unsigned int) s.value());

    if (s.level() != old_resolved_state.level())
        m_signal.raise(Signal_VoltageChange, s.level());

    bool dig_state = s.digital_value();
    if (dig_state != old_resolved_state.digital_value()) {
        m_signal.raise(Signal_DigitalChange, (unsigned char) dig_state);
    }
}

/**
   Returns the resolved state reduced to a boolean.
 */
bool Wire::digital_state() const
{
    return m_resolved_state.digital_value();
}

/**
   Copy assignment with attachment
 */
Wire& Wire::operator=(Wire& other)
{
    m_state = other.m_state;
    attach(other);
    return *this;
}

/**
   Copy assignment without attachment
 */
Wire& Wire::operator=(const Wire& other)
{
    m_state = other.m_state;
    return *this;
}
