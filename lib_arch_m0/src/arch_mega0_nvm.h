/*
 * arch_mega0_nvm.h
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

#ifndef __YASIMAVR_MEGA0_NVM_H__
#define __YASIMAVR_MEGA0_NVM_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "core/sim_memory.h"

//=======================================================================================
/*
 * Implementation of a USERROW controller for Mega0/Mega1 series
 * The purpose of this controller is only to allow reading from the userrow
 * using the data address space.
 */


class DLL_EXPORT AVR_ArchMega0_USERROW : public AVR_Peripheral {

public:

	AVR_ArchMega0_USERROW(reg_addr_t base);

	virtual bool init(AVR_Device& device) override;
	virtual void ioreg_read_handler(reg_addr_t addr) override;

private:

	const reg_addr_t m_reg_base;
	AVR_NonVolatileMemory* m_userrow;

};


//=======================================================================================
/*
 * Implementation of a fuse controller for Mega0/Mega1 series
 * The purpose of this controller is only to allow reading the fuses
 * using the data address space.
 */


class DLL_EXPORT AVR_ArchMega0_Fuses : public AVR_Peripheral {

public:

	AVR_ArchMega0_Fuses(reg_addr_t base);

	virtual bool init(AVR_Device& device) override;
	virtual void ioreg_read_handler(reg_addr_t addr) override;

private:

	const reg_addr_t m_reg_base;
	AVR_NonVolatileMemory* m_fuses;

};

#endif //__YASIMAVR_MEGA0_NVM_H__
