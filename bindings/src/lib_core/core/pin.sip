/*
 * pin.sip
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

typedef sim_id_t pin_id_t;

class Pin : public Wire /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_pin.h"
%End

public:

    struct controls_t {
        unsigned char dir /PyInt/;
        unsigned char drive /PyInt/;
        bool inverted;
        bool pull_up;

        controls_t(unsigned char /PyInt/ = 0, unsigned char /PyInt/ = 0, bool = false, bool = false);
        %MethodCode
           sipCpp = new Pin::controls_t{a0, a1, a2, a3};
        %End
    };

    Pin(pin_id_t);

    pin_id_t id() const;
    void set_external_state(const Wire::state_t&);
    void set_external_state(Wire::StateEnum, double = 0.0);
    void set_external_state(char, double = 0.0);
    void set_gpio_controls(const controls_t&);
    controls_t gpio_controls() const;

};


//=======================================================================================

class PinDriver /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_pin.h"
%End

public:

    typedef unsigned int pin_index_t;

    PinDriver(ctl_id_t, pin_index_t);

    void set_enabled(bool);
    void set_enabled(pin_index_t, bool);
    bool enabled(pin_index_t) const;

    void update_pin_state(pin_index_t);
    void update_pin_states();

    Wire::state_t pin_state(pin_index_t) const;
    Pin::controls_t gpio_controls(pin_index_t) const;

    virtual Pin::controls_t override_gpio(pin_index_t, const Pin::controls_t&) = 0;
    virtual void digital_state_changed(pin_index_t, bool);

};


//=======================================================================================

typedef UnsignedLongVector pin_id_vector_t /NoTypeName/;



class PinManager /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_pin.h"
%End

public:

    typedef sim_id_t mux_id_t;
    static const mux_id_t default_mux_id;

    PinManager(const pin_id_vector_t&);

    bool register_driver(PinDriver&);
    bool add_mux_config(ctl_id_t, const pin_id_vector_t&, mux_id_t = PinManager::default_mux_id);
    void set_current_mux(ctl_id_t, mux_id_t);
    mux_id_t current_mux(ctl_id_t) const;
    pin_id_vector_t current_mux_pins(ctl_id_t) const;
    Pin* pin(pin_id_t) const;

};
