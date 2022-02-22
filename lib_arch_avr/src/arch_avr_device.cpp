/*
 * arch_avr_device.cpp
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

#include "arch_avr_device.h"
#include <cstring>
#include "core/sim_debug.h"


//=======================================================================================

AVR_ArchAVR_Core::AVR_ArchAVR_Core(const AVR_ArchAVR_CoreConfig& config)
:AVR_Core(config)
{}

uint8_t AVR_ArchAVR_Core::cpu_read_data(mem_addr_t data_addr)
{
	uint8_t value;
	if (data_addr < 32) {
		value = m_regs[data_addr];
	}
	else if (data_addr <= m_config.ioend) {
		value = cpu_read_ioreg(data_addr - 32);
	}
	else if (data_addr >= m_config.ramstart && data_addr <= m_config.ramend) {
		value = m_sram[data_addr - m_config.ramstart];
	}
	else  {
		//TODO: log warning or crash
		value = 0;
	}

	if (m_debug_probe)
		m_debug_probe->_cpu_notify_data_read(data_addr);

	return value;
}

void AVR_ArchAVR_Core::cpu_write_data(mem_addr_t data_addr, uint8_t value)
{
	if (data_addr < 32) {
		m_regs[data_addr] = value;
	}
	else if (data_addr <= m_config.ioend) {
		cpu_write_ioreg(data_addr - 32, value);
	}
	else if (data_addr >= m_config.ramstart && data_addr <= m_config.ramend) {
		m_sram[data_addr - m_config.ramstart] = value;
	}
	else {
		//TODO: log warning or crash
	}

	if (m_debug_probe)
		m_debug_probe->_cpu_notify_data_write(data_addr);
}

void AVR_ArchAVR_Core::dbg_read_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len)
{
	std::memset(buf, 0x00, len);

	mem_addr_t bufofs, blockofs;
	uint32_t n;

	if (data_space_map(addr, len, 0, 32, &bufofs, &blockofs, &n))
		std::memcpy(buf + bufofs, m_regs + blockofs, n);

	if (data_space_map(addr, len, 32, m_config.ioend, &bufofs, &blockofs, &n)) {
		for (reg_addr_t i = 0; i < n; ++i)
			buf[bufofs + i] = cpu_read_ioreg(blockofs + i);
	}

	if (data_space_map(addr, len, m_config.ramstart, m_config.ramend, &bufofs, &blockofs, &n))
		std::memcpy(buf + bufofs, m_sram + blockofs, n);

}

void AVR_ArchAVR_Core::dbg_write_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len)
{
	mem_addr_t bufofs, blockofs;
	uint32_t n;

	if (data_space_map(addr, len, 0, 32, &bufofs, &blockofs, &n))
		std::memcpy(m_regs + blockofs, buf + bufofs, n);

	if (data_space_map(addr, len, 32, m_config.ioend, &bufofs, &blockofs, &n)) {
		for (reg_addr_t i = 0; i < n; ++i)
			cpu_write_ioreg(blockofs + i, buf[bufofs + i]);
	}

	if (data_space_map(addr, len, m_config.ramstart, m_config.ramend, &bufofs, &blockofs, &n))
		std::memcpy(m_sram + blockofs, buf + bufofs, n);
}


//=======================================================================================

AVR_ArchAVR_Device::AVR_ArchAVR_Device(const AVR_ArchAVR_DeviceConfig& config)
:AVR_Device(m_core_impl, config)
,m_core_impl(*(config.core))
{}

