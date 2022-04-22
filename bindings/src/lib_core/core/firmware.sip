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

	struct Block {
	
		size_t size /NoSetter/;
		size_t base;
		
		//uint8_t* buf;
		SIP_PYBUFFER buf /NoSetter/ {
		%GetCode
			sipPy = export_to_pybuffer(sipAPI_core, sipCpp->buf, sipCpp->size);
		%End
		};
		
		Block(size_t base, SIP_PYBUFFER data);
		%MethodCode
			sipCpp->base = a0;
			sipCpp->size = import_from_pybuffer(sipAPI_core, &(sipCpp->buf), a1);
		%End
		
	};

	char* variant /NoSetter/ {
		%GetCode
			sipPy = PyUnicode_FromString(sipCpp->variant.c_str());
		%End
	};
	
	uint32_t frequency;
	double vcc;
	double aref;
	
	std::vector<AVR_Firmware::Block> flashblocks;
	uint32_t flashsize /NoSetter/;
	uint32_t datasize;
	uint32_t bsssize;
	
	reg_addr_t console_register;
	
	AVR_Firmware();
	~AVR_Firmware();
	
	static AVR_Firmware* read_elf(const char*);
	
	SIP_PYTUPLE load_flash(flash_addr_t flashend) const
		[bool (uint8_t*, uint8_t*, flash_addr_t)];
	%MethodCode
		uint8_t* flash_buf = (uint8_t*) sipMalloc(a0 + 1);
		uint8_t* tag_buf = (uint8_t*) sipMalloc(a0 + 1);
		bool res = sipCpp->load_flash(flash_buf, tag_buf, a0);
		if (res) {
			sipRes = Py_BuildValue("y#,y#", flash_buf, a0 + 1, tag_buf, a0 + 1);
		} else {
			sipRes = Py_None;
			Py_INCREF(sipRes);
		}
		sipFree(flash_buf);
		sipFree(tag_buf);
	%End

};
