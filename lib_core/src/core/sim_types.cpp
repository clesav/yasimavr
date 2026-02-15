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
#include <cstring>
#include <climits>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

static int _bitcount(uint8_t mask)
{
    int n = 0;
    while (mask) {
        if (mask & 1) n++;
        mask >>= 1;
    }
    return n;
}


//=======================================================================================

bitmask_t::bitmask_t(uint8_t b, uint8_t m)
:bit(b)
,mask(m)
{}

bitmask_t::bitmask_t(uint8_t b)
:bitmask_t(b, 1 << b)
{}

bitmask_t::bitmask_t()
:bitmask_t(0, 0)
{}

bitmask_t::bitmask_t(const bitmask_t& bm)
:bitmask_t(bm.bit, bm.mask)
{}

bitmask_t::bitmask_t(const regbit_t& rb)
:bitmask_t(rb.bit, rb.mask)
{}

bitmask_t& bitmask_t::operator=(const bitmask_t& bm)
{
    bit = bm.bit;
    mask = bm.mask;
    return *this;
}

bitmask_t& bitmask_t::operator=(const regbit_t& rb)
{
    bit = rb.bit;
    mask = rb.mask;
    return *this;
}

int bitmask_t::bitcount() const
{
    return _bitcount(mask);
}


//=======================================================================================

regbit_t::regbit_t(reg_addr_t a, uint8_t b, uint8_t m)
:addr(a)
,bit(b)
,mask(m)
{}

regbit_t::regbit_t(reg_addr_t a, uint8_t b)
:regbit_t(a, b, 1 << b)
{}

regbit_t::regbit_t(reg_addr_t a)
:regbit_t(a, 0, 0xFF)
{}

regbit_t::regbit_t()
:regbit_t(INVALID_REGISTER, 0, 0)
{}

regbit_t::regbit_t(reg_addr_t a, const bitmask_t& bm)
:regbit_t(a, bm.bit, bm.mask)
{}

regbit_t::regbit_t(const regbit_t& rb)
:regbit_t(rb.addr, rb.bit, rb.mask)
{}

regbit_t& regbit_t::operator=(const regbit_t& rb)
{
    addr = rb.addr;
    bit = rb.bit;
    mask = rb.mask;
    return *this;
}

int regbit_t::bitcount() const
{
    return _bitcount(mask);
}


//=======================================================================================

regbit_compound_t::regbit_compound_t(const regbit_t& rb)
{
    add(rb);
}

regbit_compound_t::regbit_compound_t(const std::vector<regbit_t>& v)
{
    *this = v;
}

regbit_compound_t::regbit_compound_t(const regbit_compound_t& other)
{
    *this = other;
}

void regbit_compound_t::add(const regbit_t& rb)
{
    if (m_regbits.size())
        m_offsets.push_back(m_offsets.back() + m_regbits.back().bitcount());
    else
        m_offsets.push_back(0);

    m_regbits.push_back(rb);
}

bool regbit_compound_t::addr_match(reg_addr_t addr) const
{
    for (auto& rb : m_regbits)
        if (rb.addr == addr)
            return true;
    return false;
}

uint64_t regbit_compound_t::compound(uint8_t regvalue, size_t index) const
{
    uint64_t v = m_regbits[index].extract(regvalue);
    return v << m_offsets[index];
}

uint8_t regbit_compound_t::extract(uint64_t v, size_t index) const
{
    uint8_t rv = (v >> m_offsets[index]) & 0xFF;
    return rv & (m_regbits[index].mask >> m_regbits[index].bit);
}

int regbit_compound_t::bitcount() const
{
    int n = 0;
    for (auto& rb : m_regbits)
        n += rb.bitcount();
    return n;
}

regbit_compound_t& regbit_compound_t::operator=(const std::vector<regbit_t>& v)
{
    m_regbits.clear();
    m_offsets.clear();

    for (auto& rb : v)
        add(rb);

    return *this;
}

regbit_compound_t& regbit_compound_t::operator=(const regbit_compound_t& other)
{
    m_regbits = other.m_regbits;
    m_offsets = other.m_offsets;
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
