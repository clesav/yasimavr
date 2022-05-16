/*
 * arch_mega0_nvm.cpp
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

#include "arch_mega0_nvm.h"
#include "arch_mega0_device.h"
#include "arch_mega0_io.h"
#include "arch_mega0_io_utils.h"


//=======================================================================================

AVR_ArchMega0_USERROW::AVR_ArchMega0_USERROW(reg_addr_t base)
:AVR_Peripheral(AVR_ID('U', 'R', 'O', 'W'))
,m_reg_base(base)
,m_userrow(nullptr)
{}

bool AVR_ArchMega0_USERROW::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	//Obtain the pointer to the userrow block in RAM
	ctlreq_data_t req = { .index = AVR_ArchMega0_Core::NVM_USERROW };
	if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
		return false;
	m_userrow = reinterpret_cast<AVR_NonVolatileMemory*>(req.data.as_ptr());

	//Allocate a register in read-only access for each byte of the userrow block
	for (unsigned int i = 0; i < sizeof(USERROW_t); ++i)
		add_ioreg(m_reg_base + i, 0xFF, true);

	return status;
}

void AVR_ArchMega0_USERROW::ioreg_read_handler(reg_addr_t addr)
{
	write_ioreg(addr, (*m_userrow)[addr - m_reg_base]);
}


//=======================================================================================

AVR_ArchMega0_Fuses::AVR_ArchMega0_Fuses(reg_addr_t base)
:AVR_Peripheral(AVR_ID('F', 'U', 'S', 'E'))
,m_reg_base(base)
,m_fuses(nullptr)
{}

bool AVR_ArchMega0_Fuses::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	//Obtain the pointer to the fuse block in RAM
	ctlreq_data_t req = { .index = AVR_ArchMega0_Core::NVM_Fuses };
	if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
		return false;
	m_fuses = reinterpret_cast<AVR_NonVolatileMemory*>(req.data.as_ptr());

	//Allocate a register in read-only access for each fuse
	for (unsigned int i = 0; i < sizeof(FUSE_t); ++i)
		add_ioreg(m_reg_base + i, 0xFF, true);

	return status;
}

void AVR_ArchMega0_Fuses::ioreg_read_handler(reg_addr_t addr)
{
	write_ioreg(addr, (*m_fuses)[addr - m_reg_base]);
}
