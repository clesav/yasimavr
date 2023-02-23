/*
 * firmware.sip
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

class AVR_Firmware {
%TypeHeaderCode
#include "core/sim_firmware.h"

%End

public:

    struct Block {

        mem_block_t mem_block;
        size_t base;

    };

    std::string variant;
    uint32_t frequency;
    double vcc;
    double aref;

    reg_addr_t console_register;

    AVR_Firmware();
    AVR_Firmware(const AVR_Firmware& other);

    static AVR_Firmware* read_elf(const std::string&);

    void add_block(const std::string&, const mem_block_t&, size_t = 0);
    bool has_memory(const std::string&) const;
    size_t memory_size(const std::string&) const;
    std::vector<AVR_Firmware::Block> blocks(const std::string&) const;
    bool load_memory(const std::string&, AVR_NonVolatileMemory&) const;
    mem_addr_t datasize() const;
    mem_addr_t bsssize() const;

};
