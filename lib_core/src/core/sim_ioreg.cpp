/*
 * sim_ioreg.cpp
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

#include "sim_ioreg.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================
enum REG_Flags {
    Reg_Flag_Core = 0x01,
    Reg_Flag_Dispatcher = 0x02,
};

//=======================================================================================
/*
 * Dispatcher is a class for the few registers that are shared between several
 * peripherals. It is allocated automatically by the IOREG instances
 */
class IORegDispatcher : public IORegHandler {

public:

    void add_handler(IORegHandler& handler)
    {
        for (auto h : m_handlers)
            if (h == &handler) return;
        m_handlers.push_back(&handler);
    }

    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override
    {
        for (auto handler : m_handlers)
            value = handler->ioreg_read_handler(addr, value);

        return value;
    }

    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& value) override
    {
        for (auto handler : m_handlers)
            handler->ioreg_write_handler(addr, value);
    }

    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) override
    {
        for (auto handler : m_handlers)
            value = handler->ioreg_read_handler(addr, value);

        return value;
    }

private:

    std::vector<IORegHandler*> m_handlers;

};


//=======================================================================================

/**
   Build a register.
   \param core_reg set to true if the register is handled by the core. The effect
   is to bypass read-only/use masks checks
*/
IORegister::IORegister(bool core_reg)
:m_value(0)
,m_handler(nullptr)
,m_flags(core_reg ? Reg_Flag_Core : 0)
,m_use_mask(0)
,m_ro_mask(0)
,m_strobe_mask(0)
{}


IORegister::IORegister(const IORegister& other)
:m_value(other.m_value)
,m_flags(other.m_flags)
,m_use_mask(other.m_use_mask)
,m_ro_mask(other.m_ro_mask)
,m_strobe_mask(other.m_strobe_mask)
{
    if (m_flags & Reg_Flag_Dispatcher)
        m_handler = new IORegDispatcher(*static_cast<IORegDispatcher*>(other.m_handler));
    else
        m_handler = other.m_handler;
}


IORegister::~IORegister()
{
    if (m_flags & Reg_Flag_Dispatcher)
        delete static_cast<IORegDispatcher*>(m_handler);
}

/**
   Add a handler to this register.
   \param handler handler to add
 */
void IORegister::add_handler(IORegHandler& handler)
{
    if (m_flags & Reg_Flag_Dispatcher) {
        IORegDispatcher* dispatcher = static_cast<IORegDispatcher*>(m_handler);
        dispatcher->add_handler(handler);
    }
    else if (m_handler) {
        if (&handler != m_handler) {
            IORegDispatcher* dispatcher = new IORegDispatcher();
            dispatcher->add_handler(*m_handler);
            dispatcher->add_handler(handler);
            m_handler = dispatcher;
            m_flags |= Reg_Flag_Dispatcher;
        }
    }
    else {
        m_handler = &handler;
    }
}


void IORegister::add_bits(bitmask_t mask, BitMode mode)
{
    mask &= ~m_use_mask;
    m_use_mask |= mask;
    if (mode == RO)
        m_ro_mask |= mask;
    else if (mode == Strobe)
        m_strobe_mask |= mask;
}

/**
   CPU read access to the register.
   The handlers' read callbacks are called then the register value is returned.
 */
uint8_t IORegister::cpu_read(reg_addr_t addr)
{
    if (m_handler)
        m_value = m_handler->ioreg_read_handler(addr, m_value);

    return m_value;
}

/**
   CPU write access to the register.
   The handlers' write callbacks are called.
   \return true if the read-only rule has been violated, i.e. attempting to write
   a read-only or unused bit with '1'.(temporary removed)
   \note that if the register has no handler, all 8 bits are read-only except if
   core_reg was true at construction
 */
void IORegister::cpu_write(reg_addr_t addr, uint8_t value)
{
    if (m_handler) {
        //Ensure the unused bits are zero
        value &= m_use_mask;
        //Protect the read-only from overwriting
        value = (value & ~m_ro_mask) | (m_value & m_ro_mask);
        //Precalculate the value structure given to the handlers
        ioreg_write_t w = { value, m_value };
        //Clear the strobe bits that are being written to 1
        value = (value & ~m_strobe_mask) | (~value & m_value & m_strobe_mask);
        //Store the resulting value
        m_value = value;
        //Call the handlers
        m_handler->ioreg_write_handler(addr, w);
    }
    else if (m_flags & Reg_Flag_Core) {
        m_value = value;
    }
}

/**
   I/O peripheral interface for read/write operation on this register
 */
uint8_t IORegister::ioctl_read(reg_addr_t addr)
{
    return m_value;
}

/**
   I/O peripheral interface for read/write operation on this register
 */
void IORegister::ioctl_write(reg_addr_t addr, uint8_t value)
{
    m_value = value;
}

/**
   I/O peripheral interface for peek operation (for debug purpose) on this register
 */
uint8_t IORegister::dbg_peek(reg_addr_t addr)
{
    if (m_handler)
        m_value = m_handler->ioreg_peek_handler(addr, m_value);

    return m_value;
}
