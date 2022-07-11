/*
 * sim_memory.cpp
 *
 *	Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

//=======================================================================================

#define ADJUST_BASE_LEN(base, len, size)	\
	if ((base) >= (size))					\
		(base) = (size) - 1;				\
	if (((base) + (len)) > (size))			\
		(len) = (size) - (base);


AVR_NonVolatileMemory::AVR_NonVolatileMemory(const size_t size)
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

AVR_NonVolatileMemory::~AVR_NonVolatileMemory()
{
	if (m_size) {
		free(m_memory);
		free(m_tag);
	}
}

void AVR_NonVolatileMemory::erase()
{
	erase(0, m_size);
}

void AVR_NonVolatileMemory::erase(size_t base, size_t len)
{
	if (!m_size || !len) return;

	ADJUST_BASE_LEN(base, len, m_size);

	memset(m_memory + base, 0xFF, len);
	memset(m_tag + base, 0, len);
}

void AVR_NonVolatileMemory::erase(unsigned char* buf, size_t pos, size_t len)
{
	if (!m_size || !len) return;
	
	ADJUST_BASE_LEN(pos, len, m_size);
	
	for (size_t i = 0; i < len; ++i) {
		if (buf[i]) {
			m_memory[pos + i] = 0xFF;
			m_tag[pos + i] = 0;
		}
	}
}

bool AVR_NonVolatileMemory::program(const mem_block_t& mem_block, size_t base)
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

mem_block_t AVR_NonVolatileMemory::block() const
{
	return block(0, m_size);
}

mem_block_t AVR_NonVolatileMemory::block(size_t base, size_t size) const
{
	mem_block_t b;

	ADJUST_BASE_LEN(base, size, m_size);

	b.size = size;
	b.buf = size ? (m_memory + base) : nullptr;

	return b;
}

int AVR_NonVolatileMemory::dbg_read(size_t pos) const
{
	if (pos < m_size)
		return m_memory[pos];
	else
		return -1;
}

size_t AVR_NonVolatileMemory::dbg_read(unsigned char* buf, size_t base, size_t len) const
{
	if (!m_size || !len) return 0;

	ADJUST_BASE_LEN(base, len, m_size);

	memcpy(buf, m_memory + base, len);
	
	return len;
}

void AVR_NonVolatileMemory::dbg_write(unsigned char v, size_t pos)
{
	if (pos < m_size)
		m_memory[pos] = v;
}

void AVR_NonVolatileMemory::dbg_write(unsigned char* buf, size_t base, size_t len)
{
	if (!m_size || !len) return;

	ADJUST_BASE_LEN(base, len, m_size);

	memcpy(m_memory + base, buf, len);
}

void AVR_NonVolatileMemory::spm_write(unsigned char v, size_t pos)
{
	if (pos < m_size) {
		m_memory[pos] &= v;
		m_tag[pos] = 1;
	}
}

void AVR_NonVolatileMemory::spm_write(unsigned char* buf, unsigned char* bufset,
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
