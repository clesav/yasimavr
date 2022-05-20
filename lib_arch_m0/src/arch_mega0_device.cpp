/*
 * arch_mega0_device.cpp
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

#include "arch_mega0_device.h"
#include "core/sim_debug.h"
#include "core/sim_firmware.h"
#include <cstring>


//=======================================================================================

AVR_ArchMega0_Core::AVR_ArchMega0_Core(const AVR_ArchMega0_CoreConfig& config)
:AVR_Core(config)
,m_eeprom(config.eepromend ? (config.eepromend + 1) : 0)
,m_userrow(config.userrowend ? (config.userrowend + 1) : 0)
{}

uint8_t AVR_ArchMega0_Core::cpu_read_data(mem_addr_t data_addr)
{
	const AVR_ArchMega0_CoreConfig& cfg = reinterpret_cast<const AVR_ArchMega0_CoreConfig&>(m_config);
	uint8_t value = 0;

	if (data_addr <= cfg.ioend) {
		value = cpu_read_ioreg(data_addr);
	}
	else if (data_addr >= cfg.ramstart && data_addr <= cfg.ramend) {
		value = m_sram[data_addr - cfg.ramstart];
	}
	else if (data_addr >= cfg.eepromstart_ds && data_addr <= cfg.eepromend_ds) {
		value = m_eeprom[data_addr - cfg.eepromstart_ds];
	}
	else if (data_addr >= cfg.flashstart_ds && data_addr <= cfg.flashend_ds) {
		value = cpu_read_flash(data_addr - cfg.flashstart_ds);
	}
	else if (!m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO)) {
		ERROR_LOG(m_device->logger(), "CPU reading an invalid data address: 0x%04x", data_addr);
		m_device->crash(CRASH_BAD_CPU_IO, "Bad data address");
	}

	if (m_debug_probe)
		m_debug_probe->_cpu_notify_data_read(data_addr);

	return value;
}

void AVR_ArchMega0_Core::cpu_write_data(mem_addr_t data_addr, uint8_t value)
{
	const AVR_ArchMega0_CoreConfig& cfg = reinterpret_cast<const AVR_ArchMega0_CoreConfig&>(m_config);

	if (data_addr <= cfg.ioend) {
		cpu_write_ioreg(data_addr, value);
	}
	else if (data_addr >= cfg.ramstart && data_addr <= cfg.ramend) {
		m_sram[data_addr - cfg.ramstart] = value;
	}
	else if (data_addr >= cfg.eepromstart_ds && data_addr <= cfg.eepromend_ds) {
		//Prepare the NVM Write request
		NVM_request_t nvm_req = {
			.addr = data_addr - cfg.eepromstart_ds, //translate the address into EEPROM space
			.data = value,
			.instr = m_pc,
		};
		//Send a request to write in the eeprom
		ctlreq_data_t d = { .index = 1, .p = &nvm_req };
		m_device->ctlreq(AVR_IOCTL_NVM, AVR_CTLREQ_NVM_WRITE, &d);
	}
	else if (data_addr >= cfg.flashstart_ds && data_addr <= cfg.flashend_ds) {
		//Prepare the NVM Write request
		NVM_request_t nvm_req = {
			.addr = data_addr - cfg.flashstart_ds, //translate the address into flash space
			.data = value,
			.instr = m_pc,
		};
		//Send a request to write in the flash
		ctlreq_data_t d = { .index = 0, .p = &nvm_req };
		m_device->ctlreq(AVR_IOCTL_NVM, AVR_CTLREQ_NVM_WRITE, &d);
	}
	else if (!m_device->test_option(AVR_Device::Option_IgnoreBadCpuIO)) {
		ERROR_LOG(m_device->logger(), "CPU writing an invalid data address: 0x%04x", data_addr);
		m_device->crash(CRASH_BAD_CPU_IO, "Bad data address");
	}

	if (m_debug_probe)
		m_debug_probe->_cpu_notify_data_write(data_addr);
}

void AVR_ArchMega0_Core::dbg_read_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len)
{
	const AVR_ArchMega0_CoreConfig& cfg = reinterpret_cast<const AVR_ArchMega0_CoreConfig&>(m_config);

	std::memset(buf, 0x00, len);

	mem_addr_t bufofs, blockofs;
	uint32_t n;

	if (data_space_map(addr, len, 0, cfg.ioend, &bufofs, &blockofs, &n)) {
		for (reg_addr_t i = 0; i < n; ++i)
			buf[bufofs + i] = cpu_read_ioreg(blockofs + i);
	}

	if (data_space_map(addr, len, cfg.ramstart, cfg.ramend, &bufofs, &blockofs, &n))
		std::memcpy(buf + bufofs, m_sram + blockofs, n);
	
	if (data_space_map(addr, len, cfg.flashstart_ds, cfg.flashend_ds, &bufofs, &blockofs, &n)) {
		mem_block_t b = m_flash.block(blockofs, n);
		std::memcpy(buf + bufofs, b.buf, b.size);
	}

	if (data_space_map(addr, len, cfg.eepromstart_ds, cfg.eepromend_ds, &bufofs, &blockofs, &n)) {
		mem_block_t b = m_eeprom.block(blockofs, n);
		std::memcpy(buf + bufofs, b.buf, n);
	}
}

void AVR_ArchMega0_Core::dbg_write_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len)
{
	const AVR_ArchMega0_CoreConfig& cfg = reinterpret_cast<const AVR_ArchMega0_CoreConfig&>(m_config);

	mem_addr_t bufofs, blockofs;
	uint32_t n;

	if (data_space_map(addr, len, 0, cfg.ioend, &bufofs, &blockofs, &n)) {
		for (reg_addr_t i = 0; i < n; ++i)
			cpu_write_ioreg(blockofs + i, buf[bufofs + i]);
	}

	if (data_space_map(addr, len, cfg.ramstart, cfg.ramend, &bufofs, &blockofs, &n))
		std::memcpy(m_sram + blockofs, buf + bufofs, n);

	if (data_space_map(addr, len, cfg.flashstart_ds, cfg.flashend_ds, &bufofs, &blockofs, &n)) {
		mem_block_t b = { .size = n, .buf = buf + bufofs };
		m_flash.program(b, blockofs);
	}
	
	if (data_space_map(addr, len, cfg.eepromstart_ds, cfg.eepromend_ds, &bufofs, &blockofs, &n)) {
		mem_block_t b = { .size = n, .buf = buf + bufofs };
		m_eeprom.program(b, blockofs);
	}
}


//=======================================================================================

AVR_ArchMega0_Device::AVR_ArchMega0_Device(const AVR_ArchMega0_DeviceConfig& config)
:AVR_Device(m_core_impl, config)
,m_core_impl(*reinterpret_cast<const AVR_ArchMega0_CoreConfig*>(config.core))
{}

bool AVR_ArchMega0_Device::core_ctlreq(uint16_t req, ctlreq_data_t* reqdata)
{
	if (req == AVR_CTLREQ_CORE_NVM) {
		if (reqdata->index == AVR_ArchMega0_Core::NVM_EEPROM)
			reqdata->data = &(m_core_impl.m_eeprom);
		else if (reqdata->index == AVR_ArchMega0_Core::NVM_USERROW)
			reqdata->data = &(m_core_impl.m_userrow);
		else
			return AVR_Device::core_ctlreq(req, reqdata);
		
		return true;
	} else {
		return AVR_Device::core_ctlreq(req, reqdata);
	}
}

bool AVR_ArchMega0_Device::program(const AVR_Firmware& firmware)
{
	if (!AVR_Device::program(firmware))
		return false;
	
	if (firmware.has_memory("eeprom")) {
		if (firmware.load_memory("eeprom", m_core_impl.m_eeprom)) {
			DEBUG_LOG(logger(), "Firmware load: EEPROM loaded", "");
		} else {
			ERROR_LOG(logger(), "Firmware load: Error loading the EEPROM", "");
			return false;
		}
	}
	
	if (firmware.has_memory("user_signatures")) {
		if (firmware.load_memory("user_signatures", m_core_impl.m_userrow)) {
			DEBUG_LOG(logger(), "Firmware load: USERROW loaded", "");
		} else {
			ERROR_LOG(logger(), "Firmware load: Error loading the USERROW", "");
			return false;
		}
	}
	
	return true;
}
