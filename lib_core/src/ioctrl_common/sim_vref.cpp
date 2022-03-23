/*
 * sim_vref.cpp
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

#include "sim_vref.h"
#include "../core/sim_device.h"


AVR_IO_VREF::AVR_IO_VREF()
:AVR_Peripheral(AVR_IOCTL_VREF)
,m_vcc(0.0)
,m_aref(0.0)
{}

AVR_IO_VREF::~AVR_IO_VREF()
{}

bool AVR_IO_VREF::ctlreq(uint16_t req, ctlreq_data_t* data)
{
	if (req == AVR_CTLREQ_ADC_GET_VREF || req == AVR_CTLREQ_ACP_GET_VREF) {
		if (m_vcc == 0.0) {
			device()->crash(CRASH_INVALID_CONFIG, "VCC not set for analog operations.");
			return true;
		}

		if (data->index == Source_Ext_VCC)
			data->data = m_vcc;
		else if (data->index == Source_Ext_AVCC)
			data->data = 1.0;
		else if (data->index == Source_Ext_AREF)
			data->data = m_aref;
		else if (data->index == Source_Internal)
			data->data = get_reference(req == AVR_CTLREQ_ADC_GET_VREF ? User_ADC : User_ACP) / m_vcc;
		else
			return false;

		return true;
	}

	else if (req == AVR_CTLREQ_VREF_SET) {
		if (data->index == Source_Ext_VCC) {
			m_vcc = data->d;
			if (m_vcc < 0.0) m_vcc = 0.0;
		}
		else if (data->index == Source_Ext_AREF) {
			m_aref = data->d;
			if (m_aref > 1.0) m_aref = 1.0;
			if (m_aref < 0.0) m_aref = 0.0;
		}
		else
			return false;

		return true;
	}

	return false;
}
