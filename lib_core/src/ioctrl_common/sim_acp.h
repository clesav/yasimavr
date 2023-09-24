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
/**
   \brief Generic ACP definitions

   Definition of enumerations, configuration structures and signal Ids used for ACP models,
   common to all architectures.
 */
class AVR_CORE_PUBLIC_API ACP {

public:

    /**
       Enum definition for the ACP channel configuration
     */
    enum Channel {
        /// External pin analog input
        Channel_Pin,
        /// Internal DAC voltage
        Channel_AcompRef
    };

    struct channel_config_t : base_reg_config_t {
        /// Channel type
        Channel type;
        /// Pin ID used for external pin analog inputs
        pin_id_t pin;
    };

    enum SignalId {
        /// Raised when the output state of the comparator has change. The data is the new state.
        Signal_Output,
        /// Raised when the internal DAC value (if the peripheral has one) has changed. The data
        /// is the DAC voltage value
        Signal_DAC
    };

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_ACP_H__
