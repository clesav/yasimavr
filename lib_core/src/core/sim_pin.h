/*
 * sim_pin.h
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

#ifndef __YASIMAVR_PIN_H__
#define __YASIMAVR_PIN_H__

#include "sim_wire.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

typedef sim_id_t pin_id_t;


/**
   \brief MCU pin model.

   Pin represents a external pad of the MCU used for GPIO.
   The pin has two electrical states given by the external circuit and the internal circuit (the GPIO port),
   which are resolved into a single electrical state. In case of conflict, the SHORTED state is
   set.
 */
class AVR_CORE_PUBLIC_API Pin : public Wire {

public:

    struct controls_t {

        unsigned char dir = 0;
        unsigned char drive = 0;
        bool inverted = false;
        bool pull_up = false;

    };

    explicit Pin(pin_id_t id);

    pin_id_t id() const;

    void set_external_state(const state_t& state);
    void set_external_state(StateEnum state, double level = 0.0);
    void set_external_state(char state, double level = 0.0);

    void set_gpio_controls(const controls_t& controls);
    controls_t gpio_controls() const;

    Pin(const Pin&) = delete;
    Pin& operator=(const Pin&) = delete;

private:

    pin_id_t m_id;
    controls_t m_gpio_controls;
    state_t m_gpio_state;

    virtual state_t state_for_resolution() const override;

    void update_pin_state();

};


inline pin_id_t Pin::id() const
{
    return m_id;
}


inline void Pin::set_external_state(const state_t& state)
{
    set_state(state);
}


inline void Pin::set_external_state(StateEnum state, double level)
{
    set_state(state, level);
}


inline void Pin::set_external_state(char state, double level)
{
    set_state(state, level);
}


inline Pin::controls_t Pin::gpio_controls() const
{
    return m_gpio_controls;
}

YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_PIN_H__
