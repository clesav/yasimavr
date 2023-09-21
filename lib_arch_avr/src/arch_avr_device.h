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

typedef CoreConfiguration ArchAVR_CoreConfig;
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
    };

    explicit ArchAVR_Core(const ArchAVR_CoreConfig& config);

protected:

    virtual uint8_t cpu_read_data(mem_addr_t data_addr) override;
    virtual void cpu_write_data(mem_addr_t data_addr, uint8_t value) override;

    virtual void dbg_read_data(mem_addr_t start, uint8_t* buf, mem_addr_t len) override;
    virtual void dbg_write_data(mem_addr_t start, const uint8_t* buf, mem_addr_t len) override;

private:

    NonVolatileMemory m_eeprom;

friend class ArchAVR_Device;

};


//=======================================================================================
/**
   \brief Implementation of a MCU for AVR series
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Device : public Device {

public:

    explicit ArchAVR_Device(const ArchAVR_DeviceConfig& config);
    virtual ~ArchAVR_Device();

protected:

    virtual bool core_ctlreq(ctlreq_id_t req, ctlreq_data_t* reqdata) override;

    /// Override to load the EEPROM
    virtual bool program(const Firmware& firmware) override;

private:

    ArchAVR_Core m_core_impl;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_DEVICE_H__
