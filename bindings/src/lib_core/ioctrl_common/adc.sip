/*
 * adc.sip
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

%ModuleCode

#include "ioctrl_common/sim_adc.h"

const uint16_t CTLREQ_ADC_SET_TEMP = AVR_CTLREQ_ADC_SET_TEMP;

%End

const uint16_t CTLREQ_ADC_SET_TEMP;


//=======================================================================================

enum ADC_Channel {
	ADC_SingleEnded,
	ADC_Differential,
	ADC_Zero,
	ADC_BandGap,
	ADC_Temperature,
	ADC_AcompRef
};


struct ADC_channel_config_t : base_reg_config_t {
%TypeHeaderCode
#include "ioctrl_common/sim_adc.h"
%End

	ADC_Channel type;
	uint32_t pin_p;
	uint32_t pin_n;

};


enum ADC_SignalPhase /BaseType=IntEnum/ {
	ADC_ConversionStarted,
	ADC_AboutToSample,
	ADC_ConversionComplete
};
