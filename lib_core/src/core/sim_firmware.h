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
/*
 * Firmware contains the information of a firmware loaded from a ELF file.
 * the ELF format decoding relies on the library libelf.
 * The data from the ELF is split is the various memory areas of the device.
 * Each memory area can avec several blocks of data (e.g. flash has .text, .rodata, ...)
 * placed at different, not necessarily contiguous addresses.
 * The currently supported memory areas :
 *    [area name]           [ELF section(s)]            [LMA origin]
 *  - "flash"               .text, .data, .rodata       0
 *  - "eeprom"              .eeprom                     0x810000
 *  - "fuse"                .fuse                       0x820000
 *  - "lock"                .lock                       0x830000
 *  - "signature"           .signature                  0x840000
 *  - "user_signatures"     .user_signatures            0x850000
 */
class AVR_CORE_PUBLIC_API Firmware {

public:

    struct Block {

        mem_block_t mem_block;
        size_t base;

    };

    //These attributes can be assigned freely
    std::string                 variant;
    unsigned long               frequency;
    double                      vcc;
    double                      aref;

    reg_addr_t                  console_register;

    Firmware();
    //Copy constructor : make a deep copy of all memory blocks
    Firmware(const Firmware& other);
    ~Firmware();

    //Reads a ELF file and returns a new firmware object
    static Firmware* read_elf(const std::string& filename);

    //Add a data block to a memory
    void add_block(const std::string& name, const mem_block_t& block, size_t base = 0);

    bool has_memory(const std::string& name) const;

    size_t memory_size(const std::string& name) const;

    std::vector<Block> blocks(const std::string& name) const;

    bool load_memory(const std::string& name, NonVolatileMemory& memory) const;

    mem_addr_t datasize() const;
    mem_addr_t bsssize() const;

    //Copy assignment
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
