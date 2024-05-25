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
NonVolatileMemory::NonVolatileMemory(size_t size)
:m_size(size)
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
:NonVolatileMemory(0)
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


//=======================================================================================

/**
   Construct a section manager.
   \param page_count number of pages covering the whole memory area
   \param page_size page size in bytes
   \param section_count number of sections
 */
MemorySectionManager::MemorySectionManager(flash_addr_t page_count, flash_addr_t page_size, unsigned int section_count)
:m_page_count(page_count)
,m_page_size(page_size)
,m_section_count(section_count)
,m_current_section(section_count)
,m_limits(section_count + 1, page_count)
,m_flags(section_count * section_count, 0x00)
,m_pages(page_count, 0x00)
{
    m_limits[0] = 0;
}


/**
   Set the section limits in page number.
   Limits must be given as an array and must be organised as :
   [ L0, L1, ..., Ln-2] where Li is the 1st page of section i+1 and n the number of sections.
   For example, with 3 sections, limits = { 16, 32 } will set Section 0 as range [0;15], Section 1 as [16;31] and
   Section 2 as [32;end].
 */
void MemorySectionManager::set_section_limits(const std::vector<flash_addr_t>& limits)
{
    for (unsigned int i = 0; i < m_section_count - 1; ++i) {
        if (i < limits.size())
            m_limits[i + 1] = limits[i];
        else
            m_limits[i + 1] = m_page_count;
    }

    invalidate_page_access_map();
}


/**
   Return the section index containing the given page number.
 */
unsigned int MemorySectionManager::page_to_section(flash_addr_t page) const
{
    //Find the section boundaries containing the given page to find the new current section
    flash_addr_t start = 0, end;
    for (unsigned int index = 0; index < m_section_count; ++index) {
        end = m_limits[index + 1];
        if (start <= page && page < end) {
            return index;
        }
        start = end;
    }
    return m_section_count - 1;
}



/**
   Set the access flags from one section to another.

   Example: set_access_flags(0, 1, Read) means that code in section 0
   can read but cannot write data located in section 1.

   \param src : Section source
   \param dst : Section destination
   \param flags : OR'ed combination of access flags
 */
void MemorySectionManager::set_access_flags(unsigned int src, unsigned int dst, uint8_t flags)
{
    unsigned int index = src * m_section_count + dst;
    m_flags[index] = (m_flags[index] & ~ACCESS_FLAGS_MASK) | (flags & ACCESS_FLAGS_MASK);

    invalidate_page_access_map();
}

/**
   Set the access flags from one section to itself.
 */
void MemorySectionManager::set_access_flags(unsigned int section, uint8_t flags)
{
    set_access_flags(section, section, flags);
}

/**
   Set the fetch flag for a section.
 */
void MemorySectionManager::set_fetch_allowed(unsigned int section, bool allowed)
{
    unsigned int index = section * (m_section_count + 1);
    if (allowed)
        m_flags[index] |= FETCH_ALLOWED;
    else
        m_flags[index] &= ~FETCH_ALLOWED;

    invalidate_page_access_map();
}

/**
   Change the current address and return the fetch access flag for the containing section.
 */
bool MemorySectionManager::fetch_address(flash_addr_t addr)
{
    flash_addr_t page = addr / m_page_size;

    if (!(m_pages[page] & CURRENT_SECTION))
        update_current_section(page);

    return m_pages[page] & FETCH_ALLOWED;
}


void MemorySectionManager::update_current_section(flash_addr_t page)
{
    unsigned int old_section = m_current_section;

    //Find the section boundaries containing the given page to find the new current section.
    flash_addr_t start = 0, end;
    for (unsigned int index = 0; index < m_section_count; ++index) {
        end = m_limits[index + 1];
        if (start <= page && page < end) {
            m_current_section = index;
            break;
        }
        start = end;
    }

    //If changing section (from a valid section), signal the exit
    if (m_current_section != old_section && old_section < m_section_count)
        m_signal.raise(Signal_Leave, old_section);

    //Update the page access map
    start = 0;
    for (unsigned int index = 0; index < m_section_count; ++index) {
        end = m_limits[index + 1];
        if (start < end) {
            //Get the access flags with src=current, dst=index
            uint8_t f = m_flags[m_current_section * m_section_count + index];
            //Set the 8th bit for the current section
            if (index == m_current_section) f |= CURRENT_SECTION;
            //Mark the section pages with the access flags
            memset(m_pages.data() + start, f, end - start);
        }
        //Move the boundaries to the next section
        start = end;
    }

    //If changing section, signal the entry
    if (m_current_section != old_section)
        m_signal.raise(Signal_Enter, m_current_section);
}


void MemorySectionManager::invalidate_page_access_map()
{
    memset(m_pages.data(), 0x00, m_page_count);
}
