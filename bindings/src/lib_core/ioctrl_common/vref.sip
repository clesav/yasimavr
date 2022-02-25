/*
 * vref.sip
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

#include "ioctrl_common/sim_vref.h"

const uint16_t CTLREQ_ADC_GET_VREF = AVR_CTLREQ_ADC_GET_VREF;
const uint16_t CTLREQ_ACP_GET_VREF = AVR_CTLREQ_ACP_GET_VREF;
const uint16_t CTLREQ_VREF_SET = AVR_CTLREQ_VREF_SET;

%End

const uint16_t CTLREQ_ADC_GET_VREF;
const uint16_t CTLREQ_ACP_GET_VREF;
const uint16_t CTLREQ_VREF_SET;


class AVR_IO_VREF : public AVR_Peripheral /Abstract/ {
%TypeHeaderCode
#include "ioctrl_common/sim_vref.h"
%End

public:

	enum Source {
		Source_Ext_VCC,
		Source_Ext_AVCC,
		Source_Ext_AREF,
		Source_Internal
	};

	enum User {
		User_ADC,
		User_ACP
	};

	AVR_IO_VREF();
	virtual ~AVR_IO_VREF();

	virtual bool ctlreq(uint16_t, ctlreq_data_t*);

protected:

	virtual float get_reference(AVR_IO_VREF::User user) const = 0;
	
	float vcc() const;
	float aref() const;

};
