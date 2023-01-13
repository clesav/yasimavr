/*
 * sim_types.cpp
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

#include "sim_types.h"
#include <cstring>


//=======================================================================================

static int _bitcount(uint8_t mask)
{
    uint8_t m = mask;
    int n = 0;
    while (m) {
        if (m && 1) n++;
        m >>= 1;
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
:regbit_t(0, 0, 0)
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
    return m_regbits[index].extract((v >> m_offsets[index]) & 0xFF);
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

std::string id_to_str(uint32_t id)
{
    char buf[5];
    buf[0] = id & 0xFF;
    buf[1] = (id >> 8) & 0xFF;
    buf[2] = (id >> 16) & 0xFF;
    buf[3] = (id >> 24) & 0xFF;
    buf[4] = 0;
    return std::string(buf);
}

uint32_t str_to_id(const char* s)
{
    //Here we use the fact that strncpy pads the destination buffer
    //with null chars if the source string is shorter than 4.
    //That gives us a unique 32-bits ID even for a short name.
    char sid[5];
    strncpy(sid, s, 4);
    return sid[0] | (sid[1] << 8) | (sid[2] << 16) | (sid[3] << 24);

}

uint32_t str_to_id(const std::string& s)
{
    return str_to_id(s.c_str());
}


//=======================================================================================

vardata_t::vardata_t() : m_type(Invalid) {}
vardata_t::vardata_t(void* p_) : m_type(Pointer), p(p_) {}
vardata_t::vardata_t(const char* s_) : m_type(String), s(s_) {}
vardata_t::vardata_t(double d_) : m_type(Double), d(d_) {}
vardata_t::vardata_t(unsigned int u_) : m_type(Uinteger), u(u_) {}
vardata_t::vardata_t(int i_) : m_type(Integer), i(i_) {}
vardata_t::vardata_t(uint8_t* b_, size_t sz) : m_type(Bytes), p(b_), m_size(sz) {}
vardata_t::vardata_t(const vardata_t& v) { *this = v; }

void* vardata_t::as_ptr() const
{
    return (m_type == Pointer) ? p : nullptr;
}

const char* vardata_t::as_str() const
{
    return (m_type == String) ? s : "";
}

double vardata_t::as_double() const
{
    return (m_type == Double) ? d : 0;
}

unsigned int vardata_t::as_uint() const
{
    return (m_type == Uinteger) ? u : 0;
}

int vardata_t::as_int() const
{
    return (m_type == Integer) ? i : 0;
}

const uint8_t* vardata_t::as_bytes() const
{
    return (m_type == Bytes) ? (const uint8_t*) p : nullptr;
}

size_t vardata_t::size() const
{
    return m_size;
}

vardata_t& vardata_t::operator=(void* p_)
{
    m_type = Pointer;
    p = p_;
    return *this;
}

vardata_t& vardata_t::operator=(const char* s_)
{
    m_type = String;
    s = s_;
    return *this;
}

vardata_t& vardata_t::operator=(double d_)
{
    m_type = Double;
    d = d_;
    return *this;
}

vardata_t& vardata_t::operator=(unsigned int u_)
{
    m_type = Uinteger;
    u = u_;
    return *this;
}

vardata_t& vardata_t::operator=(int i_)
{
    m_type = Integer;
    i = i_;
    return *this;
}

vardata_t& vardata_t::operator=(const vardata_t& v)
{
    m_type = v.m_type;
    switch (m_type) {
    case Pointer:
        p = v.p; break;
    case String:
        s = v.s; break;
    case Double:
        d = v.d; break;
    case Uinteger:
        u = v.u; break;
    case Integer:
        i = v.i; break;
    case Bytes:
        p = v.p;
        m_size = v.m_size;
        break;
    }
    return *this;
}
