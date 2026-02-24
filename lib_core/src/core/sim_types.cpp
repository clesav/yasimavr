/*
 * sim_types.cpp
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

#include "sim_types.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

bool regbit_compound_t::addr_match(reg_addr_t addr) const
{
    for (auto& rb : m_regbits)
        if (rb.addr == addr)
            return true;
    return false;
}


unsigned int regbit_compound_t::bitcount() const
{
    unsigned int n = 0;
    for (auto& rb : m_regbits)
        n += rb.bitcount();
    return n;
}


regbit_compound_t& regbit_compound_t::operator=(const std::vector<regbit_t>& v)
{
    m_regbits = v;
    return *this;
}


//=======================================================================================

bool vardata_t::operator==(const vardata_t& v) const
{
    switch (t) {
        case Invalid:
            return v.t == Invalid;
        case Pointer:
            return v.t == Pointer && p == v.p;
        case Double:
            return (v.t == Double || v.t == Uinteger || v.t == Integer) && d == v.as_double();
        case Uinteger:
            return (v.t == Uinteger || v.t == Integer) && u == v.as_uint();
        case Integer:
            return (v.t == Uinteger || v.t == Integer) && i == v.as_int();
        case Bytes:
            return v.t == Bytes && std::equal(b.begin(), b.end(), v.b.begin());
        default:
            return false;
    }
}


//=======================================================================================

std::string sim_id_t::str() const
{
    //Count the characters i.e. number of non-zero bytes
    int n = 0;
    uint64_t id = m_id;
    while ((id & 0xFF) && n++ < 8) id >>= 8;

    //copy the characters in reverse order to the char array
    char buf[9];
    id = m_id;
    for (int i = n - 1; i >= 0; --i) {
        buf[i] = id & 0xFF;
        id >>= 8;
    }

    //Set the null-ending bytes and convert to std::string
    buf[n] = 0;
    return std::string(buf);
}
