/*
 * sim_adc.h
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

#ifndef __YASIMAVR_ADC_H__
#define __YASIMAVR_ADC_H__

#include "../core/sim_peripheral.h"
#include "../core/sim_pin.h"
#include "../core/sim_types.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_adc Analog-to-Digital Converter framework
   @{
 */

/**
   \name Controller requests definition for ADC
   @{
 */

/**
   Request to set the value reported by the simulated temperature sensor
    - data set to the temperature value in °C (as a double)
 */
#define AVR_CTLREQ_ADC_SET_TEMP         1

/**
   Request to force trigger an ADC conversion. The request only has effect if
   the ADC must be enabled, idle (no conversion running) and configured
   to use an external trigger.
   No data used.
 */
#define AVR_CTLREQ_ADC_TRIGGER          2

/// @}
/// @}


//=======================================================================================
/**
   \ingroup api_adc
   \brief Generic ADC definitions

   Definition of enumerations, configuration structures and signal Ids used for ADC models,
   common to all architectures.
 */
class AVR_CORE_PUBLIC_API ADC {

public:

    /**
       Enum definition for the ADC channel configuration
     */
    enum Channel {
        /// Single-ended analog input
        Channel_SingleEnded,
        /// Differential analog input
        Channel_Differential,
        /// Ground reference channel
        Channel_Zero,
        /// Internal bandgap reference voltage
        Channel_IntRef,
        /// Internal temperature sensor voltage
        Channel_Temperature,
        /// Analog comparator reference input (used on ATMega0 and ATMega1 series)
        Channel_AcompRef
    };

    /**
       Structure for configuring one ADC channel
     */
    struct channel_config_t : base_reg_config_t {
        /// Channel type
        Channel type;
        union {
            struct {
                /// Pin ID used for single-ended channels or as positive input for differential channels
                pin_id_t pin_p;
                /// Pin ID used as negative input for differential channels, unused for other channel types
                pin_id_t pin_n;
            };
            /// Used for Channel_AcompRef, index of the ACP peripheral to get the reference value from
            char per_num;
        };

        /// Measurement gain applied to the voltage value. Must be non-zero.
        unsigned int gain;
    };

    enum SignalId {
        /// Raised at the start of a conversion
        Signal_ConversionStarted,
        /// Raised just before the ADC is sampling the inputs. Last chance to set the analog values
        /// for it to be taken into account by the current conversion.
        Signal_AboutToSample,
        /// Raised when the conversion is complete and the CPU is notified that the conversion result is ready.
        Signal_ConversionComplete,
    };

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_ADC_H__
