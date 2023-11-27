/*
 * sim_memory.h
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_MEMORY_H__
#define __YASIMAVR_MEMORY_H__

#include "sim_types.h"
#include <stddef.h>

YASIMAVR_BEGIN_NAMESPACE

//=======================================================================================


struct mem_block_t {

    size_t size = 0;
    unsigned char* buf = nullptr;

};


/**
   \brief Non-volatile memory model

   Represents a block of non-volatile memory (such as flash or eeprom) of a AVR MCU.
   It has a memory block which simulates the NVM actual storage.
   Each byte has a state unprogrammed/programmed, i.e. it
   is erased or loaded with a meaningful value.
 */
class AVR_CORE_PUBLIC_API NonVolatileMemory {

public:

    explicit NonVolatileMemory(size_t size, const std::string& name = "");
    NonVolatileMemory(const NonVolatileMemory& other);
    ~NonVolatileMemory();

    size_t size() const;
    const std::string& name() const;

    bool programmed(size_t pos) const;
    size_t programmed(unsigned char* buf, size_t base, size_t len) const;

    unsigned char operator[](size_t pos) const;

    mem_block_t block() const;
    mem_block_t block(size_t base, size_t size) const;

    bool program(const mem_block_t& mem_block, size_t base = 0);

    void erase();
    void erase(size_t base, size_t size);
    void erase(const unsigned char* buf, size_t base, size_t len);

    int dbg_read(size_t pos) const;
    size_t dbg_read(unsigned char* buf, size_t base, size_t len) const;
    void dbg_write(unsigned char v, size_t pos);
    void dbg_write(const unsigned char* buf, size_t base, size_t len);

    void spm_write(unsigned char v, size_t pos);
    void spm_write(const unsigned char* buf, const unsigned char* bufset, size_t base, size_t len);

    NonVolatileMemory& operator=(const NonVolatileMemory& other);

private:

    size_t m_size;
    unsigned char* m_memory;
    unsigned char* m_tag;
    std::string m_name;

};

/**
   Return the size of the NVM.
 */
inline size_t NonVolatileMemory::size() const
{
    return m_size;
}

/**
   Return the name of the NVM.
 */
inline const std::string& NonVolatileMemory::name() const
{
    return m_name;
}

/**
   Return the unprogrammed/programmed state of one NVM byte.
   \param pos address of the byte
   \return true if the byte is programmed, false if unprogrammed
 */
inline bool NonVolatileMemory::programmed(size_t pos) const
{
    return m_tag[pos];
}

/**
   Read a single NVM byte with no boundary checks.
   \param pos address of the byte to read
   \return the byte value
 */
inline uint8_t NonVolatileMemory::operator[](size_t pos) const
{
    return m_memory[pos];
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_MEMORY_H__
