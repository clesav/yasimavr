/*
 * arch_avr_device.cpp
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
#include "core/sim_debug.h"
#include "core/sim_peripheral.h"
#include "core/sim_firmware.h"
#include <cstring>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_Core::ArchAVR_Core(const ArchAVR_CoreConfig& config)
:Core(config)
,m_eeprom(config.eepromend ? (config.eepromend + 1) : 0, "eeprom")
{}

uint8_t ArchAVR_Core::cpu_read_data(mem_addr_t data_addr)
{
    uint8_t value = 0;

    if (data_addr < 32) {
        value = m_regs[data_addr];
    }
    else if (data_addr <= m_config.ioend) {
        value = cpu_read_ioreg(data_addr - 32);
    }
    else if (data_addr >= m_config.ramstart && data_addr <= m_config.ramend) {
        value = m_sram[data_addr - m_config.ramstart];
    }
    else if (!m_device->test_option(Device::Option_IgnoreBadCpuIO)) {
        m_device->logger().err("CPU reading an invalid data address: 0x%04x", data_addr);
        m_device->crash(CRASH_BAD_CPU_IO, "Bad data address");
    }

    if (m_debug_probe)
        m_debug_probe->_cpu_notify_data_read(data_addr, value);

    return value;
}

void ArchAVR_Core::cpu_write_data(mem_addr_t data_addr, uint8_t value)
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
    else if (!m_device->test_option(Device::Option_IgnoreBadCpuIO)) {
        m_device->logger().err("CPU writing an invalid data address: 0x%04x", data_addr);
        m_device->crash(CRASH_BAD_CPU_IO, "Bad data address");
    }

    if (m_debug_probe)
        m_debug_probe->_cpu_notify_data_write(data_addr, value);
}

void ArchAVR_Core::dbg_read_data(mem_addr_t addr, uint8_t* buf, mem_addr_t len)
{
    std::memset(buf, 0x00, len);

    mem_addr_t bufofs, blockofs;
    mem_addr_t n;

    if (data_space_map(addr, len, 0, 32, &bufofs, &blockofs, &n))
        std::memcpy(buf + bufofs, m_regs + blockofs, n);

    if (data_space_map(addr, len, 32, m_config.ioend, &bufofs, &blockofs, &n)) {
        for (mem_addr_t i = 0; i < n; ++i)
            buf[bufofs + i] = cpu_read_ioreg(blockofs + i);
    }

    if (data_space_map(addr, len, m_config.ramstart, m_config.ramend, &bufofs, &blockofs, &n))
        std::memcpy(buf + bufofs, m_sram + blockofs, n);

}

void ArchAVR_Core::dbg_write_data(mem_addr_t addr, const uint8_t* buf, mem_addr_t len)
{
    mem_addr_t bufofs, blockofs;
    mem_addr_t n;

    if (data_space_map(addr, len, 0, 32, &bufofs, &blockofs, &n))
        std::memcpy(m_regs + blockofs, buf + bufofs, n);

    if (data_space_map(addr, len, 32, m_config.ioend, &bufofs, &blockofs, &n)) {
        for (mem_addr_t i = 0; i < n; ++i)
            cpu_write_ioreg(blockofs + i, buf[bufofs + i]);
    }

    if (data_space_map(addr, len, m_config.ramstart, m_config.ramend, &bufofs, &blockofs, &n))
        std::memcpy(m_sram + blockofs, buf + bufofs, n);
}


//=======================================================================================

ArchAVR_Device::ArchAVR_Device(const ArchAVR_DeviceConfig& config)
:Device(m_core_impl, config)
,m_core_impl(config.core)
{}


ArchAVR_Device::~ArchAVR_Device()
{
    erase_peripherals();
}


bool ArchAVR_Device::core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata)
{
    if (req == AVR_CTLREQ_CORE_NVM) {
        if (reqdata->index == ArchAVR_Core::NVM_EEPROM)
            reqdata->data = &(m_core_impl.m_eeprom);
        else if (reqdata->index == Core::NVM_GetCount)
            reqdata->data = (unsigned int) (Core::NVM_CommonCount + 1);
        else
            return Device::core_ctlreq(req, reqdata);

        return true;
    } else {
        return Device::core_ctlreq(req, reqdata);
    }
}

bool ArchAVR_Device::program(const Firmware& firmware)
{
    if (!Device::program(firmware))
        return false;

    if (firmware.has_memory(Firmware::Area_EEPROM)) {
        if (firmware.load_memory(Firmware::Area_EEPROM, m_core_impl.m_eeprom)) {
            logger().dbg("Firmware load: EEPROM loaded");
        } else {
            logger().err("Firmware load: Error loading the EEPROM");
            return false;
        }
    }

    return true;
}
