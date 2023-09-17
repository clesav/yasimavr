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
      "flash"           | .text, .data, .rodata | 0x000000
      "eeprom"          | .eeprom               | 0x810000
      "fuse"            | .fuse                 | 0x820000
      "lock"            | .lock                 | 0x830000
      "signature"       | .signature            | 0x840000
      "user_signatures" | .user_signatures      | 0x850000
 */
class AVR_CORE_PUBLIC_API Firmware {

public:

    struct Block {

        mem_block_t mem_block;
        size_t base;

    };

    ///Free attribute, name of the model, not used by the simulation
    std::string variant;
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

    void add_block(const std::string& name, const mem_block_t& block, size_t base = 0);

    bool has_memory(const std::string& name) const;

    size_t memory_size(const std::string& name) const;

    std::vector<Block> blocks(const std::string& name) const;

    bool load_memory(const std::string& name, NonVolatileMemory& memory) const;

    mem_addr_t datasize() const;
    mem_addr_t bsssize() const;

    Firmware& operator=(const Firmware& other);

private:

    std::map<std::string, std::vector<Block>> m_blocks;
    mem_addr_t m_datasize;
    mem_addr_t m_bsssize;

};

inline mem_addr_t Firmware::datasize() const
{
    return m_datasize;
}

inline mem_addr_t Firmware::bsssize() const
{
    return m_bsssize;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_FIRMWARE_H__
