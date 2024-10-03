/*
 * arch_avr_device.cpp
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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
#include "arch_avr_nvm.h"
#include <cstring>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_Core::ArchAVR_Core(const ArchAVR_CoreConfig& config)
:Core(config)
,m_eeprom(config.eepromend ? (config.eepromend + 1) : 0)
,m_lockbit(1)
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

#define CORE_CONFIG reinterpret_cast<const ArchAVR_CoreConfig&>(config.core)

ArchAVR_Device::ArchAVR_Device(const ArchAVR_DeviceConfig& config)
:Device(m_core_impl, config)
,m_core_impl(CORE_CONFIG)
,m_sections(CORE_CONFIG.flash_page_count(),
            CORE_CONFIG.flash_page_size ? CORE_CONFIG.flash_page_size : (CORE_CONFIG.flashend + 1),
            Section_Count)
{
    m_core_impl.m_section_manager = &m_sections;

    //On initialisation, make the whole flash an App Section with Read&Fetch flags,
    //effectively making any access control or self-programming features disabled by default.
    //A peripheral can set them up properly later if implemented.
    m_sections.set_section_limits({ m_sections.page_count(), m_sections.page_count() });
    m_sections.set_access_flags(Section_AppRWW, MemorySectionManager::Access_Read);
    m_sections.set_fetch_allowed(Section_AppRWW, true);
}


ArchAVR_Device::~ArchAVR_Device()
{
    erase_peripherals();
}


bool ArchAVR_Device::core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata)
{
    if (req == AVR_CTLREQ_CORE_NVM) {
        if (reqdata->index == ArchAVR_Core::NVM_EEPROM)
            reqdata->data = &(m_core_impl.m_eeprom);
        else if (reqdata->index == ArchAVR_Core::NVM_Lockbit)
            reqdata->data = &(m_core_impl.m_lockbit);
        else if (reqdata->index == Core::NVM_GetCount)
            reqdata->data = (unsigned int) (Core::NVM_CommonCount + 2);
        else
            return Device::core_ctlreq(req, reqdata);

        return true;
    }
    else if (req == AVR_CTLREQ_CORE_SECTIONS) {
        reqdata->data = &m_sections;
        return true;
    }
    else {
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

    if (firmware.has_memory(Firmware::Area_Lock)) {
        if (firmware.load_memory(Firmware::Area_Lock, m_core_impl.m_lockbit)) {
            logger().dbg("Firmware load: lockbit loaded");
        } else {
            logger().err("Firmware load: Error loading the lockbit");
            return false;
        }
    }

    return true;
}

/**
   \brief Override to provide the reset vector depending on fuse settings.
   The FUSE peripheral is queried for the value of the fuse BOOTRST.\n
   If BOOTRST is 1, 0x0000 is returned.\n
   If BOOTRST is 0, the 1st address of the Boot section is returned.
 */
flash_addr_t ArchAVR_Device::reset_vector()
{
    flash_addr_t addr = 0x0000;

    //Ask the Fuse Controller for the value of BOOTRST
    //Don't use Device::ctlreq because it logs a warning if no fuse peripheral is attached which is useless log noise here.
    Peripheral* fuse_per = find_peripheral(chr_to_id('F', 'U', 'S', 'E'));
    if (fuse_per) {
        ctlreq_data_t reqdata = { .index = ArchAVR_Fuses::Fuse_BootRst };
        bool ok = fuse_per->ctlreq(AVR_CTLREQ_FUSE_VALUE, &reqdata);
        //if the BOOTRST fuse is zero, the reset vector is at the start of the Boot section. Otherwise keep the default value.
        if (ok && reqdata.data.as_int() == 0)
            addr = m_sections.section_start(Section_Boot) * m_sections.page_size();
    }

    return addr;
}
