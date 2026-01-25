/*
 * memory.sip
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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

class NonVolatileMemory {
%TypeHeaderCode
#include "core/sim_memory.h"
%End
%TypeCode
#include "utils/buffer_utils.h"
%End

public:

    NonVolatileMemory(size_t);
    NonVolatileMemory(const NonVolatileMemory&);

    size_t size() const;

    bool programmed(size_t) const;
    bytes_view_t programmed(size_t, size_t) const;

    unsigned char operator[](size_t) const /PyInt/;

    bytes_view_t view() const;
    bytes_view_t view(size_t, size_t) const;

    bool program(const bytes_view_t&, size_t = 0);

    void erase();
    void erase(size_t, size_t);
    void erase(const bytes_view_t&, size_t);

    int read(size_t) const;
    SIP_PYOBJECT read(size_t, size_t) const /TypeHint="bytes"/;
    %MethodCode
        if (a1) {
            bytes_view_t b = sipCpp->view(a0, a1);
            sipRes = PyBytes_FromStringAndSize((char*) b.data(), b.size());
        } else {
            sipRes = PyBytes_FromString("");
        }
    %End

    void write(unsigned char /PyInt/, size_t);
    void write(const bytes_view_t&, size_t);

    void spm_write(unsigned char /PyInt/, size_t);
    void spm_write(const bytes_view_t&, const bytes_view_t&, size_t);

};


class MemorySectionManager /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_memory.h"
%End

public:

    enum AccessFlag /BaseType=IntFlag/ {
        Access_Read             /PyName=Read/,
        Access_Write            /PyName=Write/,
    };

    enum SignalId /BaseType=IntEnum/ {
        Signal_Leave     /PyName=Leave/,
        Signal_Enter     /PyName=Enter/,
    };

    MemorySectionManager(flash_addr_t, flash_addr_t, unsigned int);

    flash_addr_t page_count() const;
    flash_addr_t page_size() const;
    unsigned int section_count() const;

    unsigned int current_section() const;

    void set_section_limits(const UnsignedLongVector&);
    flash_addr_t section_start(unsigned int) const;
    flash_addr_t section_end(unsigned int) const;
    flash_addr_t section_size(unsigned int) const;

    unsigned int page_to_section(flash_addr_t) const;
    unsigned int address_to_section(flash_addr_t) const;

    void set_access_flags(unsigned int, unsigned int, uint8_t);
    void set_access_flags(unsigned int, uint8_t);
    uint8_t access_flags(unsigned int, unsigned int) const;
    uint8_t access_flags(unsigned int) const;
    bool can_read(flash_addr_t) const;
    bool can_write(flash_addr_t) const;
    uint8_t address_access_flags(flash_addr_t) const;

    void set_fetch_allowed(unsigned int, bool);
    bool fetch_address(flash_addr_t);

    Signal& signal();

};
