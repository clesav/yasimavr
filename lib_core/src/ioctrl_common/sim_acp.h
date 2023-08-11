/*
 * sim_acp.h
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_ACP_H__
#define __YASIMAVR_ACP_H__

#include "../core/sim_peripheral.h"
#include "../core/sim_pin.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Configuration enumerations and structures
*/
class DLL_EXPORT ACP {

public:

    enum Channel {
        Channel_Pin,
        Channel_AcompRef
    };

    struct channel_config_t : base_reg_config_t {
        Channel type;
        pin_id_t pin;
    };

    enum SignalId {
        Signal_Output,
        Signal_DAC
    };

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_ACP_H__
