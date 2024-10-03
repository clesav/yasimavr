/*
 * arch_avr_device.h
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

#ifndef __YASIMAVR_AVR_DEVICE_H__
#define __YASIMAVR_AVR_DEVICE_H__

#include "arch_avr_globals.h"
#include "core/sim_core.h"
#include "core/sim_device.h"
#include "core/sim_interrupt.h"
#include "core/sim_types.h"
#include "core/sim_memory.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
//Variant configuration structure. Nothing to add compared to generic ones
//so we just use typedef

struct ArchAVR_CoreConfig : CoreConfiguration {

    flash_addr_t            flash_page_size;

    constexpr flash_addr_t flash_page_count() const
    {
        return flash_page_size ? ((flashend + 1) / flash_page_size) : 1;
    }

};

/**
   \brief Configuration structure for ArchXT_Core
 */
typedef DeviceConfiguration ArchAVR_DeviceConfig;


//=======================================================================================
/**
   \brief Implementation of a CPU core for AVR series
   The main addition is to handle the address mapping in data space
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Core : public Core {

public:

    /// Additional NVM enumerations
    enum ArchAVR_NVM {
        NVM_EEPROM = NVM_ArchDefined,
        NVM_Lockbit = NVM_ArchDefined + 1,
    };

    explicit ArchAVR_Core(const ArchAVR_CoreConfig& config);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t data_addr) override;
    virtual void cpu_write_data(mem_addr_t data_addr, uint8_t value) override;

    virtual void dbg_read_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) override;
    virtual void dbg_write_data(mem_addr_t start, const uint8_t* buf, mem_addr_t len) override;

private:

    NonVolatileMemory m_eeprom;
    NonVolatileMemory m_lockbit;

friend class ArchAVR_Device;

};


//=======================================================================================
/**
   \brief Implementation of a MCU for AVR series
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Device : public Device {

public:

    //Definitions for the memory section manager
    enum FlashSection {
        Section_AppRWW = 0,
        Section_AppNRWW,
        Section_Boot,
        Section_Count
    };

    enum FlashSectionFlags {
        Access_RWW            = 0x04,
        Access_IntDisabled    = 0x08,
    };

    explicit ArchAVR_Device(const ArchAVR_DeviceConfig& config);
    virtual ~ArchAVR_Device();

protected:

    virtual bool core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata) override;

    /// Override to load the EEPROM
    virtual bool program(const Firmware& firmware) override;

    virtual flash_addr_t reset_vector() override;

private:

    ArchAVR_Core m_core_impl;
    MemorySectionManager m_sections;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_DEVICE_H__
