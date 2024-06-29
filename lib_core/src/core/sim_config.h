/*
 * sim_config.h
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

#ifndef __YASIMAVR_CONFIG_H__
#define __YASIMAVR_CONFIG_H__

#include "sim_types.h"
#include <vector>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 *Structure holding core configuration parameters such as memory sizes
 */
struct CoreConfiguration {

    enum Attributes {
        //Set if flash addresses use more than 16 bits
        ExtendedAddressing = 0x01,
        //Set if the Global Interrupt Enable bit is cleared automatically on interrupt routine
        ClearGIEOnInt = 0x02
    };

    uint32_t                attributes;     //OR'ed value of Attributes flags
    mem_addr_t              iostart;        //first address of the IO register file in the data space
    mem_addr_t              ioend;          //last address of the IO register file in the data space
    mem_addr_t              ramstart;       //first address of the SRAM in the data space
    mem_addr_t              ramend;         //last address of the SRAM in the data space
    mem_addr_t              dataend;        //last address of the data space
    flash_addr_t            flashend;       //last address of the flash in the code space
    mem_addr_t              eepromend;      //last address of the EEPROM in the eeprom space
    //Registers for extended addressing (on >64Kb variants)
    reg_addr_t              rampz;
    reg_addr_t              eind;

    mem_addr_t              fusesize;
    std::vector<uint8_t>    fuses;          //Fuse bytes factory values

};


//=======================================================================================
/*
 *Structure holding device configuration parameters
 */
struct DeviceConfiguration {

    std::string                         name;
    CoreConfiguration&                  core;
    std::vector<std::string>            pins;

    inline explicit DeviceConfiguration(CoreConfiguration& _core) : core(_core) {}

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_CONFIG_H__
