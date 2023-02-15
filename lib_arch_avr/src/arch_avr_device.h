/*
 * arch_avr_device.h
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

#ifndef __YASIMAVR_AVR_DEVICE_H__
#define __YASIMAVR_AVR_DEVICE_H__

#include "core/sim_core.h"
#include "core/sim_device.h"
#include "core/sim_interrupt.h"
#include "core/sim_types.h"
#include "core/sim_memory.h"

//=======================================================================================
//Variant configuration structure. Nothing to add compared to generic ones
//so we just use typedef

typedef AVR_CoreConfiguration AVR_ArchAVR_CoreConfig;
typedef AVR_DeviceConfiguration AVR_ArchAVR_DeviceConfig;


//=======================================================================================
/*
 * Implementation of a CPU core for AVR series
 * The main addition is to handle the address mapping in data space
 */
class DLL_EXPORT AVR_ArchAVR_Core : public AVR_Core {

public:

    enum AVR_ArchAVR_NVM {
        NVM_EEPROM = NVM_ArchDefined,
    };

    explicit AVR_ArchAVR_Core(const AVR_ArchAVR_CoreConfig& config);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t data_addr) override;
    virtual void cpu_write_data(mem_addr_t data_addr, uint8_t value) override;

    virtual void dbg_read_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) override;
    virtual void dbg_write_data(mem_addr_t start, const uint8_t* buf, mem_addr_t len) override;

private:

    AVR_NonVolatileMemory m_eeprom;

friend class AVR_ArchAVR_Device;

};


//=======================================================================================
/*
 * Implementation of a MCU for AVR series
 */
class DLL_EXPORT AVR_ArchAVR_Device : public AVR_Device {

public:

    explicit AVR_ArchAVR_Device(const AVR_ArchAVR_DeviceConfig& config);
    virtual ~AVR_ArchAVR_Device();

protected:

    virtual bool core_ctlreq(uint16_t req, ctlreq_data_t* reqdata) override;

    //Override to load the EEPROM
    virtual bool program(const AVR_Firmware& firmware) override;

private:

    AVR_ArchAVR_Core m_core_impl;

};

#endif //__YASIMAVR_AVR_DEVICE_H__
