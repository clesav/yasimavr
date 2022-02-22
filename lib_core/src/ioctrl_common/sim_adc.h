/*
 * sim_adc.h
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_IO_ADC_H__
#define __YASIMAVR_IO_ADC_H__

#include "../core/sim_peripheral.h"
#include "../core/sim_types.h"


//=======================================================================================
/*
 * CTLREQ definitions
*/
#define AVR_CTLREQ_ADC_SET_TEMP			1


//=======================================================================================
/*
 * Configuration enumerations and structures
*/

enum ADC_Channel {
	ADC_SingleEnded,
	ADC_Differential,
	ADC_Zero,
	ADC_BandGap,
	ADC_Temperature,
	ADC_AcompRef
};

struct ADC_channel_config_t : base_reg_config_t {
	ADC_Channel type;
	uint32_t pin_p;
	uint32_t pin_n;
};

enum ADC_SignalPhase {
	ADC_ConversionStarted,
	ADC_AboutToSample,
	ADC_ConversionComplete
};

#endif //__YASIMAVR_IO_ADC_H__
