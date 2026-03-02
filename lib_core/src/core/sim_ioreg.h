/*
 * sim_ioreg.h
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_IOREG_H__
#define __YASIMAVR_IOREG_H__

#include "sim_types.h"
#include <vector>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   This structure is used for 'ioreg_write_handler' callbacks to hold pre-calculated
   information on the new register value.
 */
struct ioreg_write_t {
    uint8_t value;   //!<new value of the register
    uint8_t old;     //!<previous value of the register
    //indicates bits transiting from '0' to '1'
    inline uint8_t posedge() const
    {
        return value & ~old;
    }

    //indicates bits transiting from '1' to '0'
    inline uint8_t negedge() const
    {
        return old & ~value;
    }

    //indicates changed bits
    inline uint8_t anyedge() const
    {
        return value ^ old;
    }

};


//=======================================================================================
/**
   Abstract interface for I/O register handlers
   The handler is notified when the register is accessed by the CPU
   It is meant to be implemented by I/O peripherals
 */
class AVR_CORE_PUBLIC_API IORegHandler {

public:

    virtual ~IORegHandler() = default;

    /**
       Callback for a CPU I/O read access. On-the-fly modifications of the
       register content are possible.
       \param addr address of the register read, in I/O address space
       \param value current cached value of the register
       \return actual value of the register
    */
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) = 0;

    /**
       Callback for a I/O peek access. On-the-fly modifications of the
       register content are possible.
       \param addr address of the register read, in I/O address space
       \param value current cached value of the register
       \return actual value of the register
    */
    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) = 0;

    /**
       Callback for a CPU I/O write access. Note that the register has already been
       updated so it's ok for the callback to overwrite it. (a typical use case is
       implementing write-one-to-clear bits)
       \param addr address of the register read, in I/O address space
       \param data struct containing the data change.
     */
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) = 0;

};


//=======================================================================================
/**
   Simulation model for a 8-bits I/O register that is a vehicle for data transfer
   between the CPU and I/O peripherals.

   Peripherals can be added as handlers to a register to be notified of
   accesses (read or write) by the CPU.

   Each bit of the register can be marked as used/unused or read-only. this is taken
   into account on write access for error checks.
 */
class AVR_CORE_PUBLIC_API IORegister {

public:

    enum BitMode {
        RW,
        RO,
        Strobe,
    };

    explicit IORegister(bool core_reg=false);
    IORegister(const IORegister& other);
    ~IORegister();

    uint8_t value() const;
    void set(uint8_t value);

    void add_bits(bitmask_t mask, BitMode mode);

    void add_handler(IORegHandler& handler);

    uint8_t cpu_read(reg_addr_t addr);
    void cpu_write(reg_addr_t addr, uint8_t value);

    uint8_t ioctl_read(reg_addr_t addr);
    void ioctl_write(reg_addr_t addr, uint8_t value);

    uint8_t dbg_peek(reg_addr_t addr);

    IORegister& operator=(const IORegister&) = delete;

private:

    //Contains the current 8-bits value of this register
    uint8_t m_value;
    //Pointer to the register handler, which is called notified when the register is accessed by the CPU
    IORegHandler *m_handler;
    //Flag set
    uint8_t m_flags;
    //Mask indicating which bits of the register are used
    bitmask_t m_use_mask;
    //Mask indicating which bits of the register are read-only or strobe for the CPU
    bitmask_t m_ro_mask;
    bitmask_t m_strobe_mask;

};

///Simple inline interface to access the value
inline uint8_t IORegister::value() const
{
    return m_value;
}

///Simple inline interface to access the value
inline void IORegister::set(uint8_t value)
{
    m_value = value;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_IOREG_H__
