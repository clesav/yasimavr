/*
 * arch_xt_device.h
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

#ifndef __YASIMAVR_XT_DEVICE_H__
#define __YASIMAVR_XT_DEVICE_H__

#include "core/sim_core.h"
#include "core/sim_device.h"
#include "core/sim_interrupt.h"
#include "core/sim_types.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

struct ArchXT_CoreConfig : CoreConfiguration {

    mem_addr_t              flashstart_ds;      //first address of the flash in the data space
    mem_addr_t              flashend_ds;        //last address of the flash in the data space

    mem_addr_t              eepromstart_ds;     //first address of the eeprom in the data space
    mem_addr_t              eepromend_ds;       //last address of the eeprom in the data space

    mem_addr_t              userrowend;

};


//=======================================================================================

typedef DeviceConfiguration ArchXT_DeviceConfig;


//=======================================================================================

class DLL_EXPORT ArchXT_Core : public Core {

public:

    enum ArchXT_NVM {
        NVM_EEPROM = NVM_ArchDefined,
        NVM_USERROW = NVM_ArchDefined + 1,
    };

    explicit ArchXT_Core(const ArchXT_CoreConfig& variant);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t data_addr) override;
    virtual void cpu_write_data(mem_addr_t data_addr, uint8_t value) override;

    virtual void dbg_read_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) override;
    virtual void dbg_write_data(mem_addr_t start, const uint8_t* buf, mem_addr_t len) override;

private:

    NonVolatileMemory m_eeprom;
    NonVolatileMemory m_userrow;

friend class ArchXT_Device;

};


//=======================================================================================

class DLL_EXPORT ArchXT_Device : public Device {

public:

    explicit ArchXT_Device(const ArchXT_DeviceConfig& config);
    virtual ~ArchXT_Device();

protected:

    virtual bool core_ctlreq(uint16_t req, ctlreq_data_t* reqdata) override;

    //Override to load the EEPROM and the USERROW
    virtual bool program(const Firmware& firmware) override;

private:

    ArchXT_Core m_core_impl;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_DEVICE_H__