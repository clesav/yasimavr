/*
 * firmware.sip
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

struct AVR_Firmware {
%TypeHeaderCode
#include "core/sim_firmware.h"
#include "utils/buffer_utils.h"
%End

public:

	char* variant /NoSetter/ {
		%GetCode
			sipPy = PyUnicode_FromString(sipCpp->variant.c_str());
		%End
	};
	
	uint32_t frequency;
	float vcc;
	float aref;

	flash_addr_t flashbase;
	
	SIP_PYBUFFER flash /NoSetter/ {
		%GetCode
			sipPy = export_to_pybuffer(sipCpp->flash, sipCpp->flashsize);
		%End
	};
	
	void set_flash(SIP_PYBUFFER);
	%MethodCode
		sipCpp->flashsize = import_from_pybuffer(&(sipCpp->flash), a0);
	%End
	
	uint32_t flashsize /NoSetter/;
	uint32_t datasize;
	uint32_t bsssize;
	
	
	SIP_PYBUFFER eeprom /NoSetter/ {
		%GetCode
			sipPy = export_to_pybuffer(sipCpp->eeprom, sipCpp->eesize);
		%End
	};
	
	void set_eeprom(SIP_PYBUFFER);
	%MethodCode
		sipCpp->eesize = import_from_pybuffer(&(sipCpp->eeprom), a0);
	%End
	
	uint32_t eesize /NoSetter/;
	
	reg_addr_t			console_register;
	
	AVR_Firmware();
	~AVR_Firmware();
	
	static AVR_Firmware* read_elf(const char*);

};
