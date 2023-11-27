/*
 * sim_memory.cpp
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

#include "sim_memory.h"
#include <cstring>

YASIMAVR_USING_NAMESPACE

//=======================================================================================

#define ADJUST_BASE_LEN(base, len, size)    \
    if ((base) >= (size))                   \
        (base) = (size) - 1;                \
    if (((base) + (len)) > (size))          \
        (len) = (size) - (base);


/**
   Construct a non-volatile memory.
   Initialise the memory block, setting it to unprogrammed and
   filling it with the default value 0xFF.
   \param size size of the NVM in bytes
 */
NonVolatileMemory::NonVolatileMemory(size_t size, const std::string& name)
:m_size(size)
,m_name(name)
{
    if (size) {
        m_memory = (unsigned char*) malloc(m_size);
        memset(m_memory, 0xFF, m_size);
        m_tag = (unsigned char*) malloc(m_size);
        memset(m_tag, 0, m_size);
    } else {
        m_memory = m_tag = nullptr;
    }
}

/**
   Destroy a non-volatile memory.
 */
NonVolatileMemory::~NonVolatileMemory()
{
    if (m_size) {
        free(m_memory);
        free(m_tag);
    }
}


NonVolatileMemory::NonVolatileMemory(const NonVolatileMemory& other)
:NonVolatileMemory(0, nullptr)
{
    *this = other;
}


/**
   Return the unprogrammed/programmed state of the NVM into a buffer.
   Each byte in the buffer is set a value of 0 for "unprogrammed" and 1 for "programmed".
   \param buf buffer to copy the NVM programmed state into
   \param base first address to be read
   \param len length of the area to be read, in bytes
   \return length of data actually read
 */
size_t NonVolatileMemory::programmed(unsigned char* buf, size_t base, size_t len) const
{
    if (!m_size || !len) return 0;

    ADJUST_BASE_LEN(base, len, m_size);

    memcpy(buf, m_tag + base, len);

    return len;
}


/**
   Erase the entire NVM.
 */
void NonVolatileMemory::erase()
{
    erase(0, m_size);
}

/**
   Erases a NVM block, overwrite all bytes of the block
   with the default value 0xFF and set their state to unprogrammed.
   \param base first address to be erased
   \param len length of the block to be erased, in bytes
 */
void NonVolatileMemory::erase(size_t base, size_t len)
{
    if (!m_size || !len) return;

    ADJUST_BASE_LEN(base, len, m_size);

    memset(m_memory + base, 0xFF, len);
    memset(m_tag + base, 0, len);
}

/**
   Selective erasing of a NVM block.
   Bytes in the block are erased only if the corresponding byte
   in the buffer argument is non-zero.
   \param buf buffer for selecting the bytes that should be erased
   \param base first address to be erased
   \param len length of the area to be erased, in bytes
 */
void NonVolatileMemory::erase(const unsigned char* buf, size_t base, size_t len)
{
    if (!m_size || !len) return;

    ADJUST_BASE_LEN(base, len, m_size);

    for (size_t i = 0; i < len; ++i) {
        if (buf[i]) {
            m_memory[base + i] = 0xFF;
            m_tag[base + i] = 0;
        }
    }
}

/**
   Load the NVM with a 'program'.
   The memory block is copied into the NVM bytes and their state are
   set to programmed.
   \param mem_block memory block with the data to be loaded into the NVM.
   \param base first address where the memory block should be copied.
   \return true if the operation was completed.
 */
bool NonVolatileMemory::program(const mem_block_t& mem_block, size_t base)
{
    if (!m_size) return false;
    if (!mem_block.size) return true;

    size_t size = mem_block.size;

    ADJUST_BASE_LEN(base, size, m_size);

    if (size) {
        memcpy(m_memory + base, mem_block.buf, size);
        memset(m_tag + base, 1, size);
    }

    return (bool) size;
}

/**
   Return a mem_block_t struct representing the entire NVM.
 */
mem_block_t NonVolatileMemory::block() const
{
    return block(0, m_size);
}

/**
   Return a mem_block_t struct representing a block of the NVM.
 */
mem_block_t NonVolatileMemory::block(size_t base, size_t size) const
{
    mem_block_t b;

    ADJUST_BASE_LEN(base, size, m_size);

    b.size = size;
    b.buf = size ? (m_memory + base) : nullptr;

    return b;
}

/**
   Read a single NVM byte
   \param pos address of the byte to read
   \return the byte value or -1 if the address is invalid
 */
int NonVolatileMemory::dbg_read(size_t pos) const
{
    if (pos < m_size)
        return m_memory[pos];
    else
        return -1;
}

/**
   Read the memory into a buffer.
   \param buf buffer to copy the NVM data into
   \param base first address to be read
   \param len length of the area to be read, in bytes
   \return length of data actually read
 */
size_t NonVolatileMemory::dbg_read(unsigned char* buf, size_t base, size_t len) const
{
    if (!m_size || !len) return 0;

    ADJUST_BASE_LEN(base, len, m_size);

    memcpy(buf, m_memory + base, len);

    return len;
}

/**
   Write a byte of the NVM without affecting the programmed state.
   \param v data to be written
   \param pos address to be written
 */
void NonVolatileMemory::dbg_write(unsigned char v, size_t pos)
{
    if (pos < m_size)
        m_memory[pos] = v;
}

/**
   Write bytes of the NVM without affecting the programmed state.
   \param buf data to be copied into the NVM
   \param base first address to be written
   \param len length of data to write
 */
void NonVolatileMemory::dbg_write(const unsigned char* buf, size_t base, size_t len)
{
    if (!m_size || !len) return;

    ADJUST_BASE_LEN(base, len, m_size);

    memcpy(m_memory + base, buf, len);
}

/**
   Write a byte to the NVM and set its state to programmed.
   \param v value to write
   \param pos address to write, in bytes
   \note The writing is performed by a bitwise AND with the previous content of the byte.
 */
void NonVolatileMemory::spm_write(unsigned char v, size_t pos)
{
    if (pos < m_size) {
        m_memory[pos] &= v;
        m_tag[pos] = 1;
    }
}

/**
   Selectively write bytes to the NVM and set their state to programmed.
   \param buf data to be copied
   \param buftag tag for the data in 'buf'.
   When a tag byte is non-zero, the corresponding byte in buf is copied into the NVM.
   \param base first address to be written
   \param len length of data to write
   \note The writing of each byte is performed by a bitwise AND with the previous content of the byte.
   \note If buftag is set to nullptr, all bytes in 'buf' are copied.
 */
void NonVolatileMemory::spm_write(const unsigned char* buf,
                                  const unsigned char* bufset,
                                  size_t base, size_t len)
{
    if (!m_size || !len) return;

    ADJUST_BASE_LEN(base, len, m_size);

    for (size_t i = 0; i < len; ++i) {
        if (!bufset || bufset[i]) {
            m_memory[base + i] &= buf[i];
            m_tag[base + i] = 1;
        }
    }
}


NonVolatileMemory& NonVolatileMemory::operator=(const NonVolatileMemory& other)
{
    if (m_size) {
        free(m_memory);
        free(m_tag);
    }

    m_size = other.m_size;
    m_name = other.m_name;

    if (m_size) {
        m_memory = (unsigned char*) malloc(m_size);
        memcpy(m_memory, other.m_memory, m_size);
        m_tag = (unsigned char*) malloc(m_size);
        memcpy(m_tag, other.m_tag, m_size);
    } else {
        m_memory = m_tag = nullptr;
    }

    return *this;
}
