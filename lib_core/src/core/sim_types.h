/*
 * sim_types.h
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

#ifndef __YASIMAVR_TYPES_H__
#define __YASIMAVR_TYPES_H__

#include <stdint.h>
#include <string>

#ifdef YASIMAVR_DLL
	#ifdef _MSC_VER
		#define DLL_EXPORT __declspec(dllexport)
	#else
		#define DLL_EXPORT __attribute__((__visibility__("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define DLL_EXPORT __declspec(dllimport)
	#else
		#define DLL_EXPORT
	#endif
#endif

//cycle counts are signed to have -1 as invalid value
//That leaves 63 bits to count cycles, which at a MCU frequency of 20MHz
//represent more than 14000 simulated years
typedef int64_t		cycle_count_t;
typedef uint16_t	reg_addr_t;
typedef uint32_t    mem_addr_t;
typedef uint32_t    flash_addr_t;
typedef int16_t		int_vect_t;

const int64_t INVALID_CYCLE = -1;

#define BITSET(v, b) (((v) >> (b)) & 0x01)

struct regbit_t;

struct bitmask_t {

	uint8_t bit;
	uint8_t mask;

	explicit bitmask_t(uint8_t b, uint8_t m);
	explicit bitmask_t(uint8_t b);
	explicit bitmask_t(const regbit_t& rb);
	bitmask_t();
	bitmask_t(const bitmask_t& bm);

	bitmask_t& operator=(const bitmask_t& bm);
	bitmask_t& operator=(const regbit_t& rb);

	inline uint8_t extract(uint8_t value) const
	{
		return (value & mask) >> bit;
	}

	inline uint8_t set_to(uint8_t reg, uint8_t value = 0xFF) const
	{
		return reg | ((mask & value) << bit);
	}

	inline uint8_t clear_from(uint8_t reg, uint8_t value = 0xFF) const
	{
		return reg & ~((mask & value) << bit);
	}

	inline uint8_t replace(uint8_t reg, uint8_t value) const
	{
		return (reg & ~mask) | ((value << bit) & mask);
	}

	int bitcount() const;

};

struct regbit_t {

	reg_addr_t addr;
	uint8_t bit;
	uint8_t mask;

	explicit regbit_t(reg_addr_t a, uint8_t b, uint8_t m);
	explicit regbit_t(reg_addr_t a, uint8_t b);
	explicit regbit_t(reg_addr_t a, const bitmask_t& bm);
	explicit regbit_t(reg_addr_t a);
	regbit_t();
	regbit_t(const regbit_t& rb);

	regbit_t& operator=(const regbit_t& rb);

	inline bool valid() const
	{
		return addr > 0;
	}

	inline uint8_t extract(uint8_t value) const
	{
		return (value & mask) >> bit;
	}

	inline uint8_t set_to(uint8_t reg, uint8_t value = 0xFF) const
	{
		return reg | (mask & (value << bit));
	}

	inline uint8_t clear_from(uint8_t reg, uint8_t value = 0xFF) const
	{
		return reg & ~(mask & (value << bit));
	}

	inline uint8_t replace(uint8_t reg, uint8_t value) const
	{
		return (reg & ~mask) | ((value << bit) & mask);
	}

	int bitcount() const;

};


//=======================================================================================

std::string id_to_str(uint32_t id);
uint32_t str_to_id(const char* s);
uint32_t str_to_id(const std::string& s);


#define AVR_ID(_a,_b,_c,_d) \
	(((_d) << 24)|((_c) << 16)|((_b) << 8)|((_a)))


//=======================================================================================

class vardata_t {

public:

	enum Type {
		Invalid,
		Pointer,
		Double,
		Uinteger,
		Integer,
		String
	};

	vardata_t();
	vardata_t(void* p);
	vardata_t(const char* s);
	vardata_t(double d);
	vardata_t(unsigned int u);
	vardata_t(int i);
	vardata_t(const vardata_t& v);

	inline Type type() const
	{
		return m_type;
	}

	void* as_ptr() const;
	const char* as_str() const;
	double as_double() const;
	unsigned int as_uint() const;
	int as_int() const;

	vardata_t& operator=(void* p);
	vardata_t& operator=(const char* s);
	vardata_t& operator=(double d);
	vardata_t& operator=(unsigned int u);
	vardata_t& operator=(int i);
	vardata_t& operator=(const vardata_t& v);

private:

	Type m_type;

	union {
		void* p;
		double d;
		uint32_t u;
		int32_t i;
		const char* s;
	};

};


#endif //__YASIMAVR_TYPES_H__
