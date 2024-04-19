/*
 * arch_avr_nvm.sip
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

struct ArchAVR_FusesConfig {
%TypeHeaderCode
#include "arch_avr_nvm.h"
%End

    struct bootsize_config_t : base_reg_config_t {
        unsigned long boot_size;
    };

    regbit_t rb_bootsz;
    regbit_t rb_bootrst;
    regbit_t rb_bootlockbit;
    regbit_t rb_applockbit;

    flash_addr_t nrww_start;

    std::vector<ArchAVR_FusesConfig::bootsize_config_t> boot_sizes;

};


class ArchAVR_Fuses : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_nvm.h"
%End

public:

    ArchAVR_Fuses(const ArchAVR_FusesConfig&);

    virtual bool init(Device&);
    virtual void reset();

};
