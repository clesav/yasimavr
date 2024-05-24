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
#include "sim_signal.h"

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

    explicit NonVolatileMemory(size_t size);
    NonVolatileMemory(const NonVolatileMemory& other);
    ~NonVolatileMemory();

    size_t size() const;

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

};

/**
   Return the size of the NVM.
 */
inline size_t NonVolatileMemory::size() const
{
    return m_size;
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


//=======================================================================================

/**
   \brief Memory section management

   Manages a memory page range by dividing it in sections, allowing to set access control flags from one section to another.
   Sections are identified by an index and always cover the whole page range.
   Section limits are set by set_section_limits() and take an array of N-1 values, where N is the number of sections.
   For example, with N=3, and 100 pages, limits = { 16; 32 } will configure the sections limits as :
   Section 0 = [0; 15], Section 1 = [16; 31], Section 2 = [32; 99]
 */
class AVR_CORE_PUBLIC_API MemorySectionManager {

public:

    /// Generic Read/Write access flags
    enum AccessFlag {
        Access_Read = 0x01,
        Access_Write = 0x02,
    };

    /// SignalID raised by the section manager
    enum SignalId {
        /// Raised when the current address leaves a section. data is set to the section index (integer)
        Signal_Leave,
        /// Raised when the current address enters a section. data is set to the section index (integer)
        Signal_Enter,
    };

    MemorySectionManager(flash_addr_t page_count, flash_addr_t page_size, unsigned int section_count);

    flash_addr_t page_count() const;
    flash_addr_t page_size() const;
    unsigned int section_count() const;

    unsigned int current_section() const;

    //Management of section boundaries
    void set_section_limits(const std::vector<flash_addr_t>& limits);
    flash_addr_t section_start(unsigned int section) const;
    flash_addr_t section_end(unsigned int section) const;
    flash_addr_t section_size(unsigned int section) const;

    //Conversion page/address to section
    unsigned int page_to_section(flash_addr_t page) const;
    unsigned int address_to_section(flash_addr_t addr) const;

    //Access control flags management
    void set_access_flags(unsigned int src, unsigned int dst, uint8_t flags);
    void set_access_flags(unsigned int section, uint8_t flags);
    uint8_t access_flags(unsigned int section_src, unsigned int section_dst) const;
    uint8_t access_flags(unsigned int section) const;
    bool can_read(flash_addr_t addr) const;
    bool can_write(flash_addr_t addr) const;
    uint8_t address_access_flags(flash_addr_t addr) const;

    //Address fetch management (setting the current section)
    void set_fetch_allowed(unsigned int section, bool allowed);
    bool fetch_address(flash_addr_t addr);

    Signal& signal();

private:

    static const uint8_t ACCESS_FLAGS_MASK = 0x3F;
    static const uint8_t FETCH_ALLOWED     = 0x40;
    static const uint8_t CURRENT_SECTION   = 0x80;

    flash_addr_t m_page_count;
    flash_addr_t m_page_size;
    unsigned int m_section_count;
    unsigned int m_current_section;
    std::vector<flash_addr_t> m_limits;
    std::vector<uint8_t> m_flags;
    std::vector<uint8_t> m_pages;
    Signal m_signal;

    void update_current_section(flash_addr_t src);
    void invalidate_page_access_map();

};

/// Getter for the page count. (as given to the constructor)
inline flash_addr_t MemorySectionManager::page_count() const
{
    return m_page_count;
}

/// Getter for the page size in bytes. (as given to the constructor)
inline flash_addr_t MemorySectionManager::page_size() const
{
    return m_page_size;
}

/// Getter for the number of sections.
inline unsigned int MemorySectionManager::section_count() const
{
    return m_section_count;
}

/// Return the section containing the current address.
inline unsigned int MemorySectionManager::current_section() const
{
    return m_current_section;
}

/// Return the section start address.
inline flash_addr_t MemorySectionManager::section_start(unsigned int index) const
{
    return m_limits[index];
}

/// Return the section end address.
inline flash_addr_t MemorySectionManager::section_end(unsigned int index) const
{
    if (m_limits[index] < m_page_count)
        return m_limits[index + 1] - 1;
    else
        return m_page_count;
}

/// Return the size in bytes of a section.
inline flash_addr_t MemorySectionManager::section_size(unsigned int index) const
{
    return m_limits[index + 1] - m_limits[index];
}

///  Return the section index containing the given memory address.
inline unsigned int MemorySectionManager::address_to_section(flash_addr_t addr) const
{
    return page_to_section(addr / m_page_size);
}

/**
   Return the access flags currently set containing the given memory address.
   \param section_src : Section source
   \param sectiondst : Section destination
 */
inline uint8_t MemorySectionManager::access_flags(unsigned int section_src, unsigned int section_dst) const
{
    return m_flags[section_src * m_section_count + section_dst] & ACCESS_FLAGS_MASK;
}

/// Return the access flags currently set from one section to itself.
inline uint8_t MemorySectionManager::access_flags(unsigned int section) const
{
    return m_flags[section * (m_section_count + 1)] & ACCESS_FLAGS_MASK;
}

/// Return the read access flag for a given address.
inline bool MemorySectionManager::can_read(flash_addr_t addr) const
{
    return m_pages[addr / m_page_size] & Access_Read;
}

/// Return the write access flag for a given address.
inline bool MemorySectionManager::can_write(flash_addr_t addr) const
{
    return m_pages[addr / m_page_size] & Access_Write;
}

/// Return the access flags for a given address.
inline uint8_t MemorySectionManager::address_access_flags(flash_addr_t addr) const
{
    return m_pages[addr / m_page_size] & ACCESS_FLAGS_MASK;
}

/// Getter for the signal of the section manager.
inline Signal& MemorySectionManager::signal()
{
    return m_signal;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_MEMORY_H__
