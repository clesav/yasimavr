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
   \brief Configuration structure for ArchAVR_Fuses.
 */
struct ArchAVR_FusesConfig {

    struct bootsize_config_t : base_reg_config_t {
        unsigned long boot_size;
    };

    regbit_t                         rb_bootsz;
    regbit_t                         rb_bootrst;
    regbit_t                         rb_bootlockbit;
    regbit_t                         rb_applockbit;

    /// Start of NRWW (No Read-While-Write) section
    flash_addr_t                     nrww_start;
    /// Boot_sizes mapping
    std::vector<bootsize_config_t>   boot_sizes;

};

/**
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_Fuses : public Peripheral {

public:

    explicit ArchAVR_Fuses(const ArchAVR_FusesConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;

private:

    const ArchAVR_FusesConfig& m_config;

    NonVolatileMemory* m_fuses;
    MemorySectionManager* m_sections;

    void configure_sections();
    uint8_t read_fuse(const regbit_t& rb) const;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_NVM_H__
