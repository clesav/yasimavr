/*
 * arch_mega0_nvm.sip
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


class AVR_ArchMega0_USERROW : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_nvm.h"
%End

public:

	AVR_ArchMega0_USERROW(reg_addr_t base);

	virtual bool init(AVR_Device&);
	virtual void ioreg_read_handler(reg_addr_t);

};


class AVR_ArchMega0_Fuses : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_nvm.h"
%End

public:

	AVR_ArchMega0_Fuses(reg_addr_t base);

	virtual bool init(AVR_Device&);
	virtual void ioreg_read_handler(reg_addr_t);

};