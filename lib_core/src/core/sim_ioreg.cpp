/*
 * sim_ioreg.cpp
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
class AVR_IO_RegDispatcher : public AVR_IO_RegHandler {

public:

    virtual ~AVR_IO_RegDispatcher() {}

    void add_handler(AVR_IO_RegHandler* handler)
    {
        for (auto h : m_handlers)
            if (h == handler) return;
        m_handlers.insert(m_handlers.end(), handler);
    }

    virtual void ioreg_read_handler(reg_addr_t addr)
    {
        for (auto handler : m_handlers)
            handler->ioreg_read_handler(addr);
    }

    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& value)
    {
        for (auto handler : m_handlers)
            handler->ioreg_write_handler(addr, value);
    }

private:

    std::vector<AVR_IO_RegHandler*> m_handlers;

};


//=======================================================================================

AVR_IO_Register::AVR_IO_Register(bool core_reg)
:m_value(0)
,m_handler(nullptr)
,m_flags(core_reg ? Reg_Flag_Core : 0)
,m_use_mask(0)
,m_ro_mask(0)
{}

AVR_IO_Register::~AVR_IO_Register()
{
    if (m_flags & Reg_Flag_Dispatcher)
        delete (AVR_IO_RegDispatcher*) m_handler;
}

void AVR_IO_Register::set_handler(AVR_IO_RegHandler* handler, uint8_t use_mask, uint8_t ro_mask)
{
    if (m_flags & Reg_Flag_Dispatcher) {
        AVR_IO_RegDispatcher* dispatcher = reinterpret_cast<AVR_IO_RegDispatcher*>(m_handler);
        dispatcher->add_handler(handler);
    }
    else if (m_handler) {
        if (handler != m_handler) {
            AVR_IO_RegDispatcher* dispatcher = new AVR_IO_RegDispatcher();
            dispatcher->add_handler(m_handler);
            dispatcher->add_handler(handler);
            m_handler = dispatcher;
            m_flags |= Reg_Flag_Dispatcher;
        }
    }
    else {
        m_handler = handler;
    }

    m_use_mask |= use_mask;
    m_ro_mask |= ro_mask;
}

uint8_t AVR_IO_Register::cpu_read(reg_addr_t addr)
{
    if (m_handler)
        m_handler->ioreg_read_handler(addr);

    return m_value;
}

bool AVR_IO_Register::cpu_write(reg_addr_t addr, uint8_t value)
{
    if (m_handler) {
        //Are we trying to write any read-only or unused bit with a '1' ?
        //bool ro_violation = value & (m_ro_mask | ~m_use_mask);
        //Protect the read-only and unused bits from overwriting
        value = (value & m_use_mask & ~m_ro_mask) | (m_value & m_ro_mask);
        //Precalculate the rising and falling
        ioreg_write_t w = { value,
                            value & ~m_value,
                            m_value & ~value,
                            m_value };
        m_value = value;
        m_handler->ioreg_write_handler(addr, w);
        return false; //return ro_violation;
    } else if (m_flags & Reg_Flag_Core) {
        m_value = value;
        return false;
    } else {
        return value > 0;
    }
}

uint8_t AVR_IO_Register::ioctl_read(reg_addr_t addr)
{
    return m_value;
}

void AVR_IO_Register::ioctl_write(reg_addr_t addr, uint8_t value)
{
    m_value = value;
}
