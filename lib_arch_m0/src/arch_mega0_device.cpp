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
#include <cstring>


//=======================================================================================

AVR_ArchMega0_Core::AVR_ArchMega0_Core(const AVR_ArchMega0_CoreConfig& config)
:AVR_Core(config)
,m_eeprom(NULL)
{
	//Allocate the EEPROM in RAM
	if (config.eepromend > 0) {
		m_eeprom = (uint8_t*) malloc(config.eepromend + 1);
	}
}

AVR_ArchMega0_Core::~AVR_ArchMega0_Core()
{
	if (m_eeprom)
		free(m_eeprom);
}

uint8_t AVR_ArchMega0_Core::cpu_read_data(mem_addr_t data_addr)
{
	const AVR_ArchMega0_CoreConfig& cfg = reinterpret_cast<const AVR_ArchMega0_CoreConfig&>(m_config);
	uint8_t value;

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
	else {
		//TODO: log warning or crash
		value = 0;
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
	else {
		//TODO: log warning or crash
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

	if (data_space_map(addr, len, cfg.eepromstart_ds, cfg.eepromend_ds, &bufofs, &blockofs, &n))
		std::memcpy(buf + bufofs, m_eeprom + blockofs, n);

	if (data_space_map(addr, len, cfg.flashstart_ds, cfg.flashend_ds, &bufofs, &blockofs, &n))
		std::memcpy(buf + bufofs, m_flash + blockofs, n);
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

	if (data_space_map(addr, len, cfg.eepromstart_ds, cfg.eepromend_ds, &bufofs, &blockofs, &n))
		std::memcpy(m_eeprom + blockofs, buf + bufofs, n);

	if (data_space_map(addr, len, cfg.flashstart_ds, cfg.flashend_ds, &bufofs, &blockofs, &n))
		std::memcpy(m_flash + blockofs, buf + bufofs, n);
}


//=======================================================================================

AVR_ArchMega0_Device::AVR_ArchMega0_Device(const AVR_ArchMega0_DeviceConfig& config)
:AVR_Device(m_core_impl, config)
,m_core_impl(*reinterpret_cast<const AVR_ArchMega0_CoreConfig*>(config.core))
{}
