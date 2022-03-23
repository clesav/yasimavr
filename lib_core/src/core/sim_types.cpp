/*
 * sim_types.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
	if (s[0] == '\0')
		return 0;
	else
		return s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
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
	}
	return *this;
}
