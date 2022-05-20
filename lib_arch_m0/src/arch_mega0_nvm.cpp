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
#include "core/sim_device.h"
#include "cstring"

//=======================================================================================

class AVR_ArchMega0_NVM::Timer : public AVR_CycleTimer {

public:

	Timer(AVR_ArchMega0_NVM& ctl) : m_ctl(ctl) {}

	virtual cycle_count_t next(cycle_count_t when) override
	{
		m_ctl.timer_next(when);
		return 0;
	}

private:

	AVR_ArchMega0_NVM& m_ctl;

};


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


//=======================================================================================

#define REG_ADDR(reg) \
	reg_addr_t(m_config.reg_base + offsetof(NVMCTRL_t, reg))

#define REG_OFS(reg) \
	offsetof(NVMCTRL_t, reg)

AVR_ArchMega0_NVM::AVR_ArchMega0_NVM(const AVR_ArchMega0_NVM_Config& config)
:AVR_Peripheral(AVR_IOCTL_NVM)
,m_config(config)
,m_state(State_Idle)
,m_flash(nullptr)
,m_buffer(nullptr)
,m_bufset(nullptr)
,m_block(Block_Invalid)
,m_page(0)
,m_eeprom(nullptr)
,m_ee_intflag(false)
{
	m_timer = new Timer(*this);
}

AVR_ArchMega0_NVM::~AVR_ArchMega0_NVM()
{
	delete m_timer;

	if (m_buffer)
		free(m_buffer);
	if (m_bufset)
		free(m_bufset);
}

bool AVR_ArchMega0_NVM::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	//Obtain the pointer to the flash block in RAM
	ctlreq_data_t req = { .index = 0 };
	if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM_PTR, &req))
		return false;
	m_flash = (uint8_t*) req.p;

	//Obtain the pointer to the EEPROM block in RAM
	req.index = 1;
	if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM_PTR, &req))
		return false;
	m_eeprom = (uint8_t*) req.p;

	//Allocate the page buffer
	m_buffer = (uint8_t*) malloc(m_config.flash_page_size);
	m_bufset = (uint8_t*) malloc(m_config.flash_page_size);

	//Allocate the registers
	add_ioreg(REG_ADDR(CTRLA), NVMCTRL_CMD_gm);
	//CTRLB not implemented
	add_ioreg(REG_ADDR(STATUS), NVMCTRL_EEBUSY_bm | NVMCTRL_FBUSY_bm, true);
	add_ioreg(REG_ADDR(INTCTRL), NVMCTRL_EEREADY_bm);
	add_ioreg(REG_ADDR(INTFLAGS), NVMCTRL_EEREADY_bm);
	//DATA and ADDR not implemented

	status &= m_ee_intflag.init(device,
							 	DEF_REGBIT_B(INTCTRL, NVMCTRL_EEREADY),
								DEF_REGBIT_B(INTFLAGS, NVMCTRL_EEREADY),
								m_config.iv_eeready);

	return status;
}

void AVR_ArchMega0_NVM::reset()
{
	//Erase the page buffer
	clear_buffer();
	//Set the EEPROM Ready flag
	SET_IOREG(INTFLAGS, NVMCTRL_EEREADY);
	//Internals
	m_state = State_Idle;
	m_block = Block_Invalid;
	m_page = 0;
}

bool AVR_ArchMega0_NVM::ctlreq(uint16_t req, ctlreq_data_t* data)
{
	//Write request from the core when writing to a data space
	//location mapped to one of the NVM blocks
	if (req == AVR_CTLREQ_NVM_WRITE) {
		NVM_request_t* nvm_req = reinterpret_cast<NVM_request_t*>(data->p);
		write_nvm(data->index, *nvm_req);
		return true;
	}
	return false;
}

void AVR_ArchMega0_NVM::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	reg_addr_t reg_ofs = addr - m_config.reg_base;

	if (reg_ofs == REG_OFS(CTRLA)) {
		NVM_Command cmd = (Command) EXTRACT_F(data.value, NVMCTRL_CMD);
		execute_command(cmd);
	}

	else if (reg_ofs == REG_OFS(INTCTRL)) {
		m_ee_intflag.update_from_ioreg();
	}

	else if (reg_ofs == REG_OFS(INTFLAGS)) {
		m_ee_intflag.clear_flag();
	}
}

void AVR_ArchMega0_NVM::clear_buffer()
{
	memset(m_buffer, 0xFF, m_config.flash_page_size);
	memset(m_bufset, 0, m_config.flash_page_size);
}

void AVR_ArchMega0_NVM::write_nvm(int block_index, const NVM_request_t& nvm_req)
{
	//Determine the actual page size, depending on which NVM block is
	//addressed
	mem_addr_t page_size;
	if (block_index == 0) {
		m_block = Block_Flash;
		page_size = m_config.flash_page_size;
	}
	else if (block_index == 1) {
		m_block = Block_EEPROM;
		page_size = m_config.flash_page_size >> 1;
	}
	else {
		m_block = Block_Invalid;
		return;
	}

	//Write to the page buffer
	mem_addr_t page_offset = nvm_req.addr % page_size;
	m_buffer[page_offset] &= nvm_req.data;
	m_bufset[page_offset] = 1;

	//Storing the page number
	m_page = nvm_req.addr / page_size;
}

void AVR_ArchMega0_NVM::execute_command(Command cmd)
{
	cycle_count_t delay = 0;

	if (cmd == Cmd_Idle) {
		//Nothing to do
		return;
	}
	else if (cmd == Cmd_BufferErase) {
		//Clear the buffer and set the CPU halt (the delay is expressed in cycles)
		m_state = State_Halting;
		clear_buffer();
		delay = m_config.buffer_erase_delay;
	}
	else {
		unsigned int delay_usecs;

		if (cmd == Cmd_ChipErase) {
			//Erase the flash (fill it with 1's)
			size_t fsize = device()->core().config().flashend + 1;
			memset(m_flash, 0xFF, fsize);
			//Erase the eeprom (fill it with 1's)
			size_t esize = device()->core().config().eepromend + 1;
			memset(m_eeprom, 0xFF, esize);
			//Erase the page buffer
			clear_buffer();
			//Set the halt state and delay
			delay_usecs = m_config.chip_erase_delay;
			m_state = State_Halting;
		}

		else if (m_block == Block_Flash) {
			//Execute the command on the flash block
			delay_usecs = execute_flash_command(cmd);
			if (delay_usecs)
				SET_IOREG(STATUS, NVMCTRL_FBUSY);
		}

		else if (m_block == Block_EEPROM) {
			delay_usecs = execute_eeprom_command(cmd);
			if (delay_usecs)
				SET_IOREG(STATUS, NVMCTRL_EEBUSY);
		}

		delay = (device()->frequency() * delay_usecs) / 1000000L;
	}

	//Halt the core if required
	if (delay) {
		if (m_state == State_Halting) {
			ctlreq_data_t d = { .index = 0 };
			device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
		}

		device()->add_cycle_timer(m_timer, device()->cycle() + delay);
	}
}

cycle_count_t AVR_ArchMega0_NVM::execute_flash_command(Command cmd)
{
	unsigned int delay_usecs = 0;
	uint8_t* p = m_flash + m_page * m_config.flash_page_size;

	if (cmd == Cmd_PageWrite || cmd == Cmd_PageEraseWrite) {
		m_state = State_Halting;
		//Copy the entire page buffer to the flash
		memcpy(p, m_buffer, m_config.flash_page_size);
		//Erase the page buffer
		clear_buffer();
		//Determine the CPU halting delay
		if (cmd == Cmd_PageWrite)
			delay_usecs = m_config.flash_write_delay;
		else
			delay_usecs = m_config.flash_erase_write_delay;
	}

	else if (cmd == Cmd_PageErase) {
		m_state = State_Halting;
		//Erase the flash page
		memset(p, 0xFF, m_config.flash_page_size);
		//Determine the CPU halting delay
		delay_usecs = m_config.flash_erase_delay;
	}

	return delay_usecs;
}

cycle_count_t AVR_ArchMega0_NVM::execute_eeprom_command(Command cmd)
{
	unsigned int delay_usecs = 0;
	size_t page_size = m_config.flash_page_size / 2;
	uint8_t* p = m_eeprom + m_page * page_size;

	if (cmd == Cmd_PageWrite || cmd == Cmd_PageEraseWrite) {
		m_state = State_Executing;

		for (size_t i = 0; i < page_size; ++i) {
			if (m_bufset[i])
				p[i] = m_buffer[i];
		}

		if (cmd == Cmd_PageWrite)
			delay_usecs = m_config.eeprom_write_delay;
		else
			delay_usecs = m_config.eeprom_erase_write_delay;
		//Erase the page buffer
		clear_buffer();
	}

	else if (cmd == Cmd_PageErase) {
		m_state = State_Executing;

		for (size_t i = 0; i < page_size; ++i) {
			if (m_bufset[i])
				p[i] = 0xFF;
		}

		delay_usecs = m_config.eeprom_erase_delay;
		//Erase the page buffer
		clear_buffer();
	}

	return delay_usecs;
}

void AVR_ArchMega0_NVM::timer_next(cycle_count_t when)
{
	if (m_block == Flash){
		CLEAR_IOREG(STATUS, NVMCTRL_FBUSY);
	}
	else if (m_block == EEPROM) {
		CLEAR_IOREG(STATUS, NVMCTRL_EEBUSY);
		m_ee_intflag.set_flag();
	}

	if (m_state == State_Halting) {
		m_state = State_Idle;

		ctlreq_data_t d = { .index = 1 };
		device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_HALT, &d);
	}
}
