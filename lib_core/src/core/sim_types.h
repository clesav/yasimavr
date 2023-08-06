/*
 * sim_types.h
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

#ifndef __YASIMAVR_TYPES_H__
#define __YASIMAVR_TYPES_H__

#include "sim_globals.h"
#include <stdint.h>
#include <string>
#include <vector>

YASIMAVR_BEGIN_NAMESPACE

//cycle counts are signed to have -1 as invalid value
//That leaves 63 bits to count cycles, which at a MCU frequency of 20MHz
//represent more than 14000 simulated years
typedef long long        cycle_count_t;
typedef unsigned long    mem_addr_t;
typedef unsigned long    flash_addr_t;
typedef short            int_vect_t;


const cycle_count_t INVALID_CYCLE = -1;


class reg_addr_t {

public:

    constexpr inline reg_addr_t(short addr = -1) : m_addr(addr) {}

    constexpr inline bool valid() const { return m_addr >= 0; }

    constexpr inline operator short() const { return m_addr; }

private:

    short m_addr;

};

const reg_addr_t INVALID_REGISTER;


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
        return addr.valid();
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


class regbit_compound_t {

public:

    regbit_compound_t() = default;
    explicit regbit_compound_t(const regbit_t& rb);
    explicit regbit_compound_t(const std::vector<regbit_t>& v);
    regbit_compound_t(const regbit_compound_t& other);

    void add(const regbit_t& rb);

    std::vector<regbit_t>::const_iterator begin() const;
    std::vector<regbit_t>::const_iterator end() const;
    size_t size() const;
    const regbit_t& operator[](size_t index) const;

    bool addr_match(reg_addr_t addr) const;
    uint64_t compound(uint8_t regvalue, size_t index) const;
    uint8_t extract(uint64_t v, size_t index) const;

    regbit_compound_t& operator=(const std::vector<regbit_t>& v);
    regbit_compound_t& operator=(const regbit_compound_t& other);

private:

    std::vector<regbit_t> m_regbits;
    std::vector<int> m_offsets;

};

inline std::vector<regbit_t>::const_iterator regbit_compound_t::begin() const
{
    return m_regbits.begin();
}

inline std::vector<regbit_t>::const_iterator regbit_compound_t::end() const
{
    return m_regbits.end();
}

inline size_t regbit_compound_t::size() const
{
    return m_regbits.size();
}

inline const regbit_t& regbit_compound_t::operator[](size_t index) const
{
    return m_regbits[index];
}

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
        String,
        Bytes
    };

    vardata_t();
    vardata_t(void* p);
    vardata_t(const char* s);
    vardata_t(double d);
    inline vardata_t(unsigned char u) : vardata_t((unsigned long long) u) {}
    inline vardata_t(unsigned short u) : vardata_t((unsigned long long) u) {}
    inline vardata_t(unsigned int u) : vardata_t((unsigned long long) u) {}
    inline vardata_t(unsigned long u) : vardata_t((unsigned long long) u) {}
    vardata_t(unsigned long long u);
    inline vardata_t(signed char i) : vardata_t((long long) i) {}
    inline vardata_t(short i) : vardata_t((long long) i) {}
    inline vardata_t(int i) : vardata_t((long long) i) {}
    inline vardata_t(long i) : vardata_t((long long) i) {}
    vardata_t(long long i);
    vardata_t(uint8_t* b_, size_t sz);
    vardata_t(const vardata_t& v);

    inline Type type() const
    {
        return m_type;
    }

    void* as_ptr() const;
    const char* as_str() const;
    double as_double() const;
    unsigned long long as_uint() const;
    long long as_int() const;

    const uint8_t* as_bytes() const;
    size_t size() const;

    vardata_t& operator=(void* p);
    vardata_t& operator=(const char* s);
    vardata_t& operator=(double d);
    inline vardata_t& operator=(unsigned char u) {*this = (unsigned long long) u; return *this; }
    inline vardata_t& operator=(unsigned short u) {*this = (unsigned long long) u; return *this; }
    inline vardata_t& operator=(unsigned int u) {*this = (unsigned long long) u; return *this; }
    inline vardata_t& operator=(unsigned long u) {*this = (unsigned long long) u; return *this; }
    vardata_t& operator=(unsigned long long u);
    inline vardata_t& operator=(signed char i) {*this = (long long) i; return *this; }
    inline vardata_t& operator=(short i) {*this = (long long) i; return *this; }
    inline vardata_t& operator=(int i) {*this = (long long) i; return *this; }
    inline vardata_t& operator=(long i) {*this = (long long) i; return *this; }
    vardata_t& operator=(long long i);
    vardata_t& operator=(const vardata_t& v);

    bool operator==(const vardata_t& v) const;
    bool operator!=(const vardata_t& v) const;

private:

    Type m_type;

    union {
        void* p;
        double d;
        unsigned long long u;
        long long i;
        const char* s;
    };

    size_t m_size = 0;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_TYPES_H__
