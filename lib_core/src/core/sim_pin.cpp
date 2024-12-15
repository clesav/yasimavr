/*
 * sim_pin.cpp
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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


//=======================================================================================

static Wire::state_t controls_to_state(const Pin::controls_t& c)
{
    Wire::state_t s;
    if (c.dir) {
        if (c.drive ^ c.inverted)
            s = Wire::State_High;
        else
            s = Wire::State_Low;
    } else {
        if (c.pull_up)
            s = Wire::State_PullUp;
        else
            s = Wire::State_Floating;
    }

    return s;
}


Pin::Pin(pin_id_t id)
:Wire()
,m_id(id)
{}


void Pin::set_gpio_controls(const controls_t& controls)
{
    m_gpio_controls = controls;
    update_pin_state();
}


Wire::state_t Pin::state_for_resolution() const
{
    state_t ext_pin_state = Wire::state_for_resolution();
    return resolve_two_states(ext_pin_state, m_gpio_state);
}


void Pin::update_pin_state()
{
    m_gpio_state = controls_to_state(m_gpio_controls);
    auto_resolve_state();
}
