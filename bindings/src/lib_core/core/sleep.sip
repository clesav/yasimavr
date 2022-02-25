/*
 * sleep.sip
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

enum class AVR_SleepMode /BaseType=IntEnum/ {
	Invalid,
	Active,
	Pseudo,
	Idle,
	ADC,
	Standby,
	ExtStandby,
	PowerDown,
	PowerSave,
};

const char* SleepModeName(AVR_SleepMode mode);


struct AVR_SleepConfig {
%TypeHeaderCode
#include "core/sim_sleep.h"
#include "utils/buffer_utils.h"
%End

	struct mode_config_t : base_reg_config_t {
		AVR_SleepMode mode;
		
		SIP_PYBUFFER int_mask {
			%GetCode
				sipPy = sipConvertToArray(sipCpp->int_mask, "B", 16, 0);
			%End
            %SetCode
                sipErr = import_from_fixedlen_sequence(sipCpp->int_mask, "B", 16, sipPy);
            %End
        };
		
	};

	std::vector<AVR_SleepConfig::mode_config_t> modes;
	regbit_t rb_mode;
	regbit_t rb_enable;
};


class AVR_SleepController : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_sleep.h"
%End

public:

	AVR_SleepController(const AVR_SleepConfig&);
	virtual ~AVR_SleepController();

	virtual bool init(AVR_Device&);
	virtual bool ctlreq(uint16_t, ctlreq_data_t*);

};
