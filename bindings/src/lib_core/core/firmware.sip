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

class Firmware {
%TypeHeaderCode
#include "core/sim_firmware.h"
%End
%TypeCode
#include "utils/buffer_utils.h"
%End

public:

    struct Block {
        %TypeCode
        #include "utils/buffer_utils.h"
        %End

        SIP_PYOBJECT buf /NoSetter,TypeHint="bytes"/ {
        %GetCode
            sipPy = export_to_pybuffer(sipAPI_core, sipCpp->buf, sipCpp->size);
        %End
        };

        size_t base /NoSetter/;
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

    void add_block(Area, SIP_PYBUFFER, size_t = 0);
    %MethodCode
        unsigned char* buf;
        size_t len = import_from_pybuffer(sipAPI_core, &buf, a1);
        sipCpp->add_block(a0, {len, buf, a2});
        if (len) sipFree(buf);
    %End

    bool has_memory(Area) const;
    size_t memory_size(Area) const;
    std::vector<Firmware::Area> memories() const;
    std::vector<Firmware::Block> blocks(Area) const;
    bool load_memory(Area, NonVolatileMemory&) const;
    mem_addr_t datasize() const;
    mem_addr_t bsssize() const;
    void add_symbol(const Firmware::Symbol&);
    const std::vector<Firmware::Symbol>& symbols() const;

};
