/*
 * sim_types.h
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

#ifndef __YASIMAVR_TYPES_H__
#define __YASIMAVR_TYPES_H__

#include "sim_globals.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <span>
#include <climits>


YASIMAVR_BEGIN_NAMESPACE

//cycle counts are signed to have -1 as invalid value
//That leaves 63 bits to count cycles, which at a MCU frequency of 20MHz
//represent more than 14000 simulated years
typedef long long        cycle_count_t;
typedef unsigned long    mem_addr_t;
typedef unsigned long    flash_addr_t;
typedef short            int_vect_t;


const cycle_count_t INVALID_CYCLE = -1;


//=======================================================================================

typedef std::vector<uint8_t> bytes_t;
typedef std::span<const uint8_t> bytes_view_t;


//=======================================================================================

/**
   \brief Representation of a I/O register address, with validity state.
 */
class AVR_CORE_PUBLIC_API reg_addr_t {

public:

    constexpr inline reg_addr_t(short addr = -1) : m_addr(addr >= 0 ? addr : -1) {}

    constexpr inline bool valid() const { return m_addr >= 0; }

    constexpr inline operator short() const { return m_addr; }

private:

    short m_addr;

};

const reg_addr_t INVALID_REGISTER;


#define BITSET(v, b) (((v) >> (b)) & 0x01)

struct regbit_t;


/**
   \brief Bit mask structure for bitwise operations on 8-bits registers
 */
struct AVR_CORE_PUBLIC_API bitmask_t {

    uint8_t bit;
    uint8_t mask;

    explicit bitmask_t(uint8_t b, uint8_t m);
    explicit bitmask_t(uint8_t b);
    explicit bitmask_t(const regbit_t& rb);
    bitmask_t();
    bitmask_t(const bitmask_t& bm);

    bitmask_t& operator=(const bitmask_t& bm);
    bitmask_t& operator=(const regbit_t& rb);

    /**
       Extracts the field value from the register value
     */
    inline uint8_t extract(uint8_t value) const
    {
        return (value & mask) >> bit;
    }

    /**
       Performs a bitwise OR between the register value and the field value
     */
    inline uint8_t set_to(uint8_t reg, uint8_t value = 0xFF) const
    {
        return reg | ((mask & value) << bit);
    }

    /**
       Performs a bitwise NEG-AND between the register value and the field value
     */
    inline uint8_t clear_from(uint8_t reg, uint8_t value = 0xFF) const
    {
        return reg & ~((mask & value) << bit);
    }

    /**
       Replace the field value within the register value
     */
    inline uint8_t replace(uint8_t reg, uint8_t value) const
    {
        return (reg & ~mask) | ((value << bit) & mask);
    }

    /**
       Returns the number of bits in the field
     */
    int bitcount() const;

};


/**
   \brief Representation of a register field or bit position

   Defined by the I/O register address, a bit position and a mask.
 */
struct AVR_CORE_PUBLIC_API regbit_t {

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

    /**
       Returns the validity of the register address
     */
    inline bool valid() const
    {
        return addr.valid();
    }

    /**
       Extracts the field value from the register value
     */
    inline uint8_t extract(uint8_t value) const
    {
        return (value & mask) >> bit;
    }

    /**
       Performs a bitwise OR between the register value and the field value
     */
    inline uint8_t set_to(uint8_t reg, uint8_t value = 0xFF) const
    {
        return reg | (mask & (value << bit));
    }

    /**
       Performs a bitwise NEG-AND between the register value and the field value
     */
    inline uint8_t clear_from(uint8_t reg, uint8_t value = 0xFF) const
    {
        return reg & ~(mask & (value << bit));
    }

    /**
       Replace the field value within the register value
     */
    inline uint8_t replace(uint8_t reg, uint8_t value) const
    {
        return (reg & ~mask) | ((value << bit) & mask);
    }

    /**
       Returns the number of bits in the field
     */
    int bitcount() const;

};


//=======================================================================================
/**
   regbit_compound_t allows to model a register field that is split in separate locations
   Under the hood, it is an array of regbit_t, each representing a piece of the field.
   It can be iterated over, like a standard container, yielding the regbit pieces.
 */
class AVR_CORE_PUBLIC_API regbit_compound_t {

public:

    regbit_compound_t() = default;
    explicit regbit_compound_t(const regbit_t& rb);
    explicit regbit_compound_t(const std::vector<regbit_t>& v);
    regbit_compound_t(const regbit_compound_t& other);

    ///Adds a regbit piece to the end
    void add(const regbit_t& rb);

    ///Returns a iterator to the beginning of the regbit pieces
    std::vector<regbit_t>::const_iterator begin() const;

    ///Returns a iterator to the end of the regbit pieces
    std::vector<regbit_t>::const_iterator end() const;

    ///Returns the number of pieces
    size_t size() const;

    ///Returns a reference to the specified regbit
    const regbit_t& operator[](size_t index) const;

    ///Returns true if the addr matches any of the regbit pieces
    bool addr_match(reg_addr_t addr) const;

    ///Returns a compound value that can be OR's together with
    uint64_t compound(uint8_t regvalue, size_t index) const;

    ///Extracts a compound value for a specified regbit piece
    uint8_t extract(uint64_t v, size_t index) const;

    ///Returns the number of bits in the field
    int bitcount() const;

    ///Assigns the regbit pieces
    regbit_compound_t& operator=(const std::vector<regbit_t>& v);

    ///Assigns the compound
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

class AVR_CORE_PUBLIC_API vardata_t {

public:

    typedef std::exception bad_type;

    enum Type {
        Invalid,
        Pointer,
        Double,
        Uinteger,
        Integer,
        Bytes
    };

    constexpr vardata_t() : t(Invalid), i(0) {}

    template<typename T>
    constexpr vardata_t(T* p_) : t(Pointer), p(const_cast<std::remove_cv_t<T>*>(p_)) {}

    constexpr vardata_t(double d_) : t(Double), d(d_) {}

    constexpr vardata_t(unsigned char u_) : vardata_t((unsigned long long) u_) {}
    constexpr vardata_t(unsigned short u_) : vardata_t((unsigned long long) u_) {}
    constexpr vardata_t(unsigned int u_) : vardata_t((unsigned long long) u_) {}
    constexpr vardata_t(unsigned long u_) : vardata_t((unsigned long long) u_) {}
    constexpr vardata_t(unsigned long long u_) : t(Uinteger), u(u_) {}

    constexpr vardata_t(signed char i_) : vardata_t((long long) i_) {}
    constexpr vardata_t(short i_) : vardata_t((long long) i_) {}
    constexpr vardata_t(int i_) : vardata_t((long long) i_) {}
    constexpr vardata_t(long i_) : vardata_t((long long) i_) {}
    constexpr vardata_t(long long i_) : t(Integer), i(i_) {}

    constexpr vardata_t(const bytes_view_t& b_) : t(Bytes), b(b_) {}

    constexpr vardata_t(const vardata_t&) = default;
    constexpr vardata_t(vardata_t&&) = default;

    constexpr Type type() const { return t; }

    template<typename T>
    constexpr T* as_ptr() const
    {
        if (t != Pointer)
            throw bad_type();
        return reinterpret_cast<T*>(p);
    }

    constexpr double as_double() const
    {
        if (t == Double)
            return d;
        else if (t == Uinteger)
            return u;
        else if (t == Integer)
            return i;
        else
            throw bad_type();
    }

    constexpr unsigned long long as_uint() const
    {
        if (t == Uinteger)
            return u;
        else if (t == Integer && i >= 0)
            return i;
        throw bad_type();
    }

    constexpr long long as_int() const
    {
        if (t == Integer)
            return i;
        else if (t == Uinteger && u <= LLONG_MAX)
            return u;
        throw bad_type();
    }

    constexpr bytes_view_t as_bytes() const
    {
        if (t != Bytes)
            throw bad_type();
        return b;
    }

    template<typename T>
    constexpr vardata_t& operator=(T* p_) { return (*this = vardata_t(p_)); }

    constexpr vardata_t& operator=(double d_) { return (*this = vardata_t(d_)); }

    constexpr vardata_t& operator=(unsigned char u_) { return (*this = (unsigned long long) u_); }
    constexpr vardata_t& operator=(unsigned short u_) { return (*this = (unsigned long long) u_); }
    constexpr vardata_t& operator=(unsigned int u_) { return (*this = (unsigned long long) u_); }
    constexpr vardata_t& operator=(unsigned long u_) { return (*this = (unsigned long long) u_); }
    constexpr vardata_t& operator=(unsigned long long u_) { return (*this = vardata_t(u_)); }

    constexpr vardata_t& operator=(signed char i_) { return (*this = (long long) i_); }
    constexpr vardata_t& operator=(short i_) { return (*this = (long long) i_); }
    constexpr vardata_t& operator=(int i_) { return (*this = (long long) i_); }
    constexpr vardata_t& operator=(long i_) { return (*this = (long long) i_); }
    constexpr vardata_t& operator=(long long i_) { return (*this = vardata_t(i_)); }

    constexpr vardata_t& operator=(const bytes_view_t& b_) { return (*this = vardata_t(b_)); }

    constexpr vardata_t& operator=(const vardata_t&) = default;
    constexpr vardata_t& operator=(vardata_t&&) = default;

    bool operator==(const vardata_t& other) const;
    inline bool operator!=(const vardata_t& other) const { return !(*this == other); }

private:

    Type t;

    union {
        void* p;
        double d;
        long long i;
        unsigned long long u;
        bytes_view_t b;
    };

};


//=======================================================================================

/**
   \brief Representation of a ID internally represented as a 64-bits integer but can
   be initialised with a string.
 */
class AVR_CORE_PUBLIC_API sim_id_t {

public:

    constexpr sim_id_t() : m_id(0) {}
    constexpr sim_id_t(uint64_t id) : m_id(id) {}
    constexpr sim_id_t(const char* s) : m_id(strtoid(s)) {}
    constexpr sim_id_t(const std::string& s) : m_id(strtoid(s.c_str())) {}
    constexpr sim_id_t(const sim_id_t&) = default;

    constexpr sim_id_t& operator=(uint64_t id) { m_id = id; return *this; }
    constexpr sim_id_t& operator=(const char* s) { m_id = strtoid(s); return *this; }
    constexpr sim_id_t& operator=(const std::string& s) { m_id = strtoid(s.c_str()); return *this; }
    constexpr sim_id_t& operator=(const sim_id_t&) = default;

    constexpr operator vardata_t() const { return vardata_t(m_id); }

    std::string str() const;

    constexpr bool operator==(const sim_id_t& other) const { return m_id == other.m_id; }

    constexpr operator bool() const { return !!m_id; }

    constexpr sim_id_t operator+(char c) const { return sim_id_t((m_id << 8) | c); }
    constexpr sim_id_t& operator+=(char c) { m_id = (m_id << 8) | c; return *this; }

private:

    friend struct std::hash<sim_id_t>;

    uint64_t m_id;

    static constexpr uint64_t strtoid(const char* s)
    {
        uint64_t id = 0;
        int n = 0;
        while (n++ < 8 && *s)
            id = (id << 8) | *s++;
        return id;
    }

};

typedef sim_id_t ctl_id_t;


YASIMAVR_END_NAMESPACE

/**
   \brief Instantiation of the hash template for the sim_id_t class so that it can
   be used as a key in mapping containers such as std::map.
 */
template<>
struct std::hash<YASIMAVR_QUALIFIED_NAME(sim_id_t)>
{
    constexpr std::size_t operator()(const YASIMAVR_QUALIFIED_NAME(sim_id_t)& sid) const noexcept
    {
        return sid.m_id;
    }
};


#endif //__YASIMAVR_TYPES_H__
