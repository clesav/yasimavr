/*
 * arch_avr_nvm.h
 *
 *  Copyright 2024 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_AVR_NVM_H__
#define __YASIMAVR_AVR_NVM_H__

#include "core/sim_types.h"
#include "core/sim_peripheral.h"
#include "core/sim_memory.h"
#include "arch_avr_globals.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \name Controller requests definition for ArchAVR_Fuses
   @{
 */

/**
   Request to obtain the value of a fuse.

   index should be one of ArchAVR_Fuses::Fuses enum values
 */
#define AVR_CTLREQ_FUSE_VALUE          1

/// @}


/**
   \brief Configuration structure for ArchAVR_Fuses.
 */
struct ArchAVR_FusesConfig {

    struct bootsize_config_t : base_reg_config_t {
        unsigned long boot_size;
    };

    /// Regbit for the boot size fuse bits
    regbit_t                         rb_bootsz;
    /// Regbit for the boot reset fuse bit
    regbit_t                         rb_bootrst;
    /// Regbit for the boot part of the lockbits
    bitmask_t                        bm_bootlockbit;
    /// Regbit for the application part of the lockbits
    bitmask_t                        bm_applockbit;

    /// Start of NRWW (No Read-While-Write) section, in number of section pages
    flash_addr_t                     nrww_start;
    /// Boot_sizes mapping, in number of section pages
    std::vector<bootsize_config_t>   boot_sizes;

};

/**
   \brief Implementation of a fuse NVM peripheral for AVR series

   The purpose of this controller is to use the values of the fuses to configure the section
   access control flags on a device reset.
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Fuses : public Peripheral {

public:

    enum Fuses {
        Fuse_BootRst,
    };

    explicit ArchAVR_Fuses(const ArchAVR_FusesConfig& config);

    virtual bool init(Device& device) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void reset() override;

private:

    const ArchAVR_FusesConfig& m_config;

    NonVolatileMemory* m_fuses;
    NonVolatileMemory* m_lockbit;
    MemorySectionManager* m_sections;

    void configure_sections();
    uint8_t read_fuse(const regbit_t& rb) const;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_NVM_H__
