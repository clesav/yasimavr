/*
 * sim_firmware.h
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

#ifndef __YASIMAVR_FIRMWARE_H__
#define __YASIMAVR_FIRMWARE_H__

#include "sim_types.h"
#include <string>
#include <vector>

//=======================================================================================
/*
 * AVR_Firmware contains the information of a firmware loaded from a ELF file.
 * the ELF format decoding relies on the library libelf
 */
class DLL_EXPORT AVR_Firmware {

public:

	struct Block {
		size_t size;
		size_t base;
		uint8_t* buf;
	};

	std::string			variant;
	unsigned int		frequency;
	double				vcc;
	double				aref;

	std::vector<Block>	flashblocks;
	flash_addr_t		flashsize;
	mem_addr_t			datasize;
	mem_addr_t			bsssize;

	reg_addr_t			console_register;

	AVR_Firmware();
	~AVR_Firmware();

	static AVR_Firmware* read_elf(const std::string& filename);

	bool load_flash(uint8_t* flash, uint8_t* tag, flash_addr_t flashend) const;

};

#endif //__YASIMAVR_FIRMWARE_H__
