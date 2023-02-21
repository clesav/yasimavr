/*
 * config.sip
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

struct AVR_CoreConfiguration {
%TypeHeaderCode
#include "core/sim_config.h"
%End

    enum Attributes /BaseType=IntFlag/ {
        ExtendedAddressing = 0x01,
        ClearGIEOnInt = 0x02
    };

    uint32_t            attributes;
    uint8_t             vector_size;
    uint16_t            iostart;
    uint16_t            ioend;
    uint32_t            ramstart;
    uint32_t            ramend;
    uint32_t            dataend;
    uint32_t            flashend;
    uint32_t            eepromend;
    reg_addr_t          rampz;
    reg_addr_t          eind;
    mem_addr_t          fusesize;
    UnsignedInt8Vector  fuses;

};


struct AVR_DeviceConfiguration /NoDefaultCtors/ {

    std::string                     name;
    AVR_CoreConfiguration&          core;
    StdStringVector                 pins;

    AVR_DeviceConfiguration(AVR_CoreConfiguration&);

};
