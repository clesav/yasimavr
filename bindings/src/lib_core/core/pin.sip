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
        uint8_t dir;
        uint8_t drive;
        bool inverted;
        bool pull_up;

        controls_t(uint8_t = 0, uint8_t = 0, bool = false, bool = false);
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
