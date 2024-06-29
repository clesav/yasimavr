/*
 * config.sip
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

struct CoreConfiguration {
%TypeHeaderCode
#include "core/sim_config.h"
%End

    enum Attributes /BaseType=IntFlag/ {
        ExtendedAddressing = 0x01,
        ClearGIEOnInt = 0x02
    };

    uint32_t            attributes;
    mem_addr_t          iostart;
    mem_addr_t          ioend;
    mem_addr_t          ramstart;
    mem_addr_t          ramend;
    mem_addr_t          dataend;
    flash_addr_t        flashend;
    mem_addr_t          eepromend;
    reg_addr_t          rampz;
    reg_addr_t          eind;
    mem_addr_t          fusesize;
    UnsignedInt8Vector  fuses;

};


struct DeviceConfiguration /NoDefaultCtors/ {

    std::string                     name;
    CoreConfiguration&              core;
    StdStringVector                 pins;

    DeviceConfiguration(CoreConfiguration&);

};
