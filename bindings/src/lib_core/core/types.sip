/*
 * types.sip
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


typedef int64_t			cycle_count_t;
typedef uint16_t		reg_addr_t;
typedef uint32_t    	mem_addr_t;
typedef uint32_t    	flash_addr_t;
typedef int				int_vect_t;


struct bitmask_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

	uint8_t bit;
	uint8_t mask;

	bitmask_t(uint8_t, uint8_t);
	bitmask_t(uint8_t);
	bitmask_t();
	bitmask_t(const bitmask_t&);
	bitmask_t(const regbit_t&);

	uint8_t extract(uint8_t) const;
	int bitcount() const;
	
	const char* __repr__() const;
	%MethodCode
		return PyUnicode_FromFormat("Bitmask(%d, %02x)", sipCpp->bit, sipCpp->mask);
	%End

};


struct regbit_t {
%TypeHeaderCode
#include "core/sim_types.h"
%End

public:

	reg_addr_t addr;
	uint8_t bit;
	uint8_t mask;
	
	regbit_t(reg_addr_t, uint8_t, uint8_t);
	regbit_t(reg_addr_t, uint8_t);
	regbit_t(reg_addr_t);
	regbit_t();
	regbit_t(reg_addr_t, const bitmask_t&);
	
	bool valid() const;
	uint8_t extract(uint8_t) const;
	int bitcount() const;
	
	const char* __repr__() const;
	%MethodCode
		return PyUnicode_FromFormat("Regbit(%04x, %d, %02x)", sipCpp->addr, sipCpp->bit, sipCpp->mask);
	%End
	
};


const char* id_to_str(uint32_t);
%MethodCode
	sipRes = id_to_str(a0).c_str();
%End

uint32_t str_to_id(const char*);
