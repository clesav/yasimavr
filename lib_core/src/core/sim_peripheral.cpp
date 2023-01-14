/*
 * sim_peripheral.cpp
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

#include "sim_peripheral.h"
#include "sim_device.h"
#include "sim_interrupt.h"


//=======================================================================================

AVR_Peripheral::AVR_Peripheral(uint32_t id)
:m_id(id)
,m_device(nullptr)
,m_logger(id)
{
}

AVR_Peripheral::~AVR_Peripheral()
{
    if (m_device)
        m_logger.dbg("IOCTL %s destroyed", name().c_str());
}

std::string AVR_Peripheral::name() const
{
    return id_to_str(m_id);
}

bool AVR_Peripheral::init(AVR_Device& device)
{
    m_device = &device;
    m_logger.set_parent(&device.logger());
    return true;
}

void AVR_Peripheral::reset()
{}

bool AVR_Peripheral::ctlreq(uint16_t req, ctlreq_data_t* data)
{
    return false;
}

uint8_t AVR_Peripheral::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    return value;
}

void AVR_Peripheral::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& value)
{}

void AVR_Peripheral::sleep(bool on, AVR_SleepMode mode)
{}

void AVR_Peripheral::add_ioreg(const regbit_t& rb, bool readonly)
{
    m_device->add_ioreg_handler(rb, *this, readonly);
}

void AVR_Peripheral::add_ioreg(const regbit_compound_t& rbc, bool readonly)
{
    for (auto& rb : rbc)
        m_device->add_ioreg_handler(rb, *this, readonly);
}

void AVR_Peripheral::add_ioreg(reg_addr_t addr, uint8_t mask, bool readonly)
{
    regbit_t rb = regbit_t(addr, 0, mask);
    m_device->add_ioreg_handler(rb, *this, readonly);
}

uint8_t AVR_Peripheral::read_ioreg(reg_addr_t addr) const
{
    return m_device->core().ioctl_read_ioreg(addr);
}

void AVR_Peripheral::write_ioreg(const regbit_t& rb, uint8_t value)
{
    m_device->core().ioctl_write_ioreg(rb, value);
}

uint64_t AVR_Peripheral::read_ioreg(const regbit_compound_t& rbc) const
{
    uint64_t v = 0;
    for (size_t i = 0; i < rbc.size(); ++i)
        v |= rbc.compound(read_ioreg(rbc[i].addr), i);
    return v;
}

void AVR_Peripheral::write_ioreg(const regbit_compound_t& rbc, uint64_t value)
{
    for (size_t i = 0; i < rbc.size(); ++i)
        write_ioreg(rbc[i], rbc.extract(value, i));
}

void AVR_Peripheral::set_ioreg(const regbit_compound_t& rbc)
{
    for (auto& rb : rbc)
        set_ioreg(rb);
}

void AVR_Peripheral::clear_ioreg(const regbit_compound_t& rbc)
{
    for (auto& rb : rbc)
        set_ioreg(rb);
}

bool AVR_Peripheral::register_interrupt(int_vect_t vector, AVR_InterruptHandler& handler) const
{
    if (vector < 0) {
        return true;
    }
    else if (vector > 0 && m_device) {
        ctlreq_data_t d = { &handler, vector };
        return m_device->ctlreq(AVR_IOCTL_INTR, AVR_CTLREQ_INTR_REGISTER, &d);
    }
    else {
        return false;
    }
}

AVR_Signal* AVR_Peripheral::get_signal(uint32_t ctl_id) const
{
    if (m_device) {
        ctlreq_data_t d;
        bool status = m_device->ctlreq(ctl_id, AVR_CTLREQ_GET_SIGNAL, &d);
        if (status)
            return reinterpret_cast<AVR_Signal*>(d.data.as_ptr());
    }
    return nullptr;
}


//=======================================================================================

AVR_DummyController::AVR_DummyController(uint32_t id, const std::vector<dummy_register_t>& regs)
:AVR_Peripheral(id)
,m_registers(regs)
{}

bool AVR_DummyController::init(AVR_Device& device)
{
    bool status = AVR_Peripheral::init(device);
    for (dummy_register_t r : m_registers)
        add_ioreg(r.reg);
    return status;
}

void AVR_DummyController::reset()
{
    for (dummy_register_t r : m_registers)
        write_ioreg(r.reg, r.reset);
}
