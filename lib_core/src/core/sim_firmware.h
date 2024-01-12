/*
 * sim_firmware.h
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

#ifndef __YASIMAVR_FIRMWARE_H__
#define __YASIMAVR_FIRMWARE_H__

#include "sim_types.h"
#include "sim_memory.h"
#include <string>
#include <map>
#include <vector>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   Firmware contains the information of a firmware loaded from a ELF file.
   A firmware consists of blocks of binary data that can be loaded into the various
   non-volatile memory areas of a MCU.
   Each memory area can have several blocks of data (e.g. flash has .text, .rodata, ...)
   placed at different addresses, not necessarily contiguous.
   The currently supported memory areas :
      area name         |  ELF section(s)       | LMA origin
      ------------------|-----------------------|-----------
      Flash             | .text, .data, .rodata | 0x000000
      EEPROM            | .eeprom               | 0x810000
      Fuses             | .fuse                 | 0x820000
      Lock              | .lock                 | 0x830000
      Signature         | .signature            | 0x840000
      UserSignatures    | .user_signatures      | 0x850000
 */
class AVR_CORE_PUBLIC_API Firmware {

public:

    struct Block : mem_block_t {
        size_t      base = 0;
    };

    enum Area {
        Area_Flash,
        Area_Data,
        Area_EEPROM,
        Area_Fuses,
        Area_Lock,
        Area_Signature,
        Area_UserSignatures,
    };

    struct Symbol {
        size_t      addr;
        size_t      size;
        std::string name;
        Area        area;
    };

    ///Main clock frequency in hertz, mandatory to run the simulation.
    unsigned long frequency;
    ///Power supply voltage in volts. If not set, analog peripherals such as ADC are not usable.
    double vcc;
    ///Analog reference voltage in volts
    double aref;
    ///I/O register address used for console output
    reg_addr_t console_register;

    Firmware();
    Firmware(const Firmware& other);
    ~Firmware();

    static Firmware* read_elf(const std::string& filename);

    void add_block(Area area, const Block& block);

    bool has_memory(Area area) const;

    std::vector<Area> memories() const;

    size_t memory_size(Area area) const;

    std::vector<Block> blocks(Area area) const;

    bool load_memory(Area area, NonVolatileMemory& memory) const;

    mem_addr_t datasize() const;
    mem_addr_t bsssize() const;

    void add_symbol(const Symbol& s);
    const std::vector<Symbol>& symbols() const;

    Firmware& operator=(const Firmware& other);

private:

    std::map<Area, std::vector<Block>> m_blocks;
    mem_addr_t m_datasize;
    mem_addr_t m_bsssize;
    std::vector<Symbol> m_symbols;

};

inline mem_addr_t Firmware::datasize() const
{
    return m_datasize;
}

inline mem_addr_t Firmware::bsssize() const
{
    return m_bsssize;
}

inline const std::vector<Firmware::Symbol>& Firmware::symbols() const
{
    return m_symbols;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_FIRMWARE_H__
