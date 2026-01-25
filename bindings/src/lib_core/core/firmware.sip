/*
 * firmware.sip
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

class Firmware {
%TypeHeaderCode
#include "core/sim_firmware.h"
%End
%TypeCode
#include "utils/buffer_utils.h"
%End

public:

    struct block_view_t {
        bytes_view_t bytes;
        size_t base;
    };

    enum Area {
        Area_Flash           /PyName=Flash/,
        Area_Data            /PyName=Data/,
        Area_EEPROM          /PyName=EEPROM/,
        Area_Fuses           /PyName=Fuses/,
        Area_Lock            /PyName=Lock/,
        Area_Signature       /PyName=Signature/,
        Area_UserSignatures  /PyName=UserSignatures/,
    };

    struct Symbol {
        size_t      addr;
        size_t      size;
        std::string name;
        Area        area;
    };

    unsigned long frequency;
    double vcc;
    double aref;

    reg_addr_t console_register;

    Firmware();
    Firmware(const Firmware&);

    static Firmware* read_elf(const std::string&);

    void add_block(Area, const block_view_t&);
    bool has_memory(Area) const;
    size_t memory_size(Area) const;
    std::vector<Firmware::Area> memories() const;
    std::vector<Firmware::block_view_t> blocks(Area) const;
    bool load_memory(Area, NonVolatileMemory&) const;
    mem_addr_t datasize() const;
    mem_addr_t bsssize() const;
    void add_symbol(const Firmware::Symbol&);
    const std::vector<Firmware::Symbol>& symbols() const;

};
