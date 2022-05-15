/*
 * memory.sip
 *
 *	Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

%MappedType mem_block_t {
%TypeHeaderCode
#include "core/sim_memory.h"
#include "utils/buffer_utils.h"
%End

%ConvertFromTypeCode
	return export_to_pybuffer(sipAPI_core, sipCpp->buf, sipCpp->size);
%End

%ConvertToTypeCode
    if (!sipIsErr) {
		return PyObject_CheckBuffer(sipPy);
	}
	
	mem_block_t* block = new mem_block_t();
	block->size = import_from_pybuffer(sipAPI_core, &(block->buf), sipPy);
	
	if (PyErr_Occurred()) {
		*sipIsErr = 1;
		delete block;
		return 0;
	} else {
		*sipCppPtr = block;
		return sipGetState(sipTransferObj);
	}
%End

};


class AVR_NonVolatileMemory /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_memory.h"
%End

public:

	AVR_NonVolatileMemory(const size_t size);
	~AVR_NonVolatileMemory();
	
	size_t size() const;
	
	bool programmed(size_t) const;
	
	unsigned char operator[](size_t) const /PyInt/;
	
	void erase();
	void erase(size_t, size_t);
	
	bool program(const mem_block_t&, size_t = 0);
	
	mem_block_t block() const;
	mem_block_t block(size_t, size_t) const;
	
	void write(unsigned char v /PyInt/, size_t pos);
	void write(SIP_PYBUFFER, size_t pos);
	%MethodCode
		unsigned char* buf;
		size_t len = import_from_pybuffer(sipAPI_core, &buf, a0);
		sipCpp->write(buf, a1, len);
	%End

};
