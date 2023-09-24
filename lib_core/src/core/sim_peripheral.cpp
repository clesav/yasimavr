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

YASIMAVR_USING_NAMESPACE


//=======================================================================================

/**
   Build a default peripheral.
*/
Peripheral::Peripheral(ctl_id_t id)
:m_id(id)
,m_device(nullptr)
,m_logger(id)
{
}

Peripheral::~Peripheral()
{
    if (m_device)
        m_logger.dbg("IOCTL %s destroyed", name().c_str());
}

/**
   \return the name of the peripheral (the id converted to 4 ASCII characters)
*/
std::string Peripheral::name() const
{
    return id_to_str(m_id);
}

/**
   Virtual method called when the device is initialised. This is where the peripheral can
   allocate its I/O registers, interrupts or connect signals.
   \return boolean indicates the success of all allocations.
*/
bool Peripheral::init(Device& device)
{
    m_device = &device;
    m_logger.set_parent(&device.logger());
    return true;
}

/**
   Virtual method called when the device is reset. Note that resetting I/O registers is only
   necessary here if their reset value is not zero.
*/
void Peripheral::reset()
{}

/**
   Virtual method called for a CTL request. The method must return true if the request has
   been processed.
*/
bool Peripheral::ctlreq(ctlreq_id_t, ctlreq_data_t*)
{
    return false;
}

/**
   Virtual method called when the CPU is reading a I/O register allocated by this peripheral.
   The value has not been read yet so the module can modify it before the CPU gets it.
   \param addr the register address in I/O space
*/
uint8_t Peripheral::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    return value;
}


/**
   Virtual method called when the CPU is writing a I/O register allocated by this peripheral.
   The value has already been written.
   \param addr the register address in I/O space
   \param value the new register content
*/
void Peripheral::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& value)
{}

/**
   Virtual method called when the device enters or exits a sleep mode.
   \param on true when entering a sleep mode, false when exiting it.
   \param mode one of the enum SleepMode values
*/
void Peripheral::sleep(bool on, SleepMode mode)
{}

void Peripheral::add_ioreg(const regbit_t& rb, bool readonly)
{
    m_device->add_ioreg_handler(rb, *this, readonly);
}

void Peripheral::add_ioreg(const regbit_compound_t& rbc, bool readonly)
{
    for (auto& rb : rbc)
        m_device->add_ioreg_handler(rb, *this, readonly);
}

void Peripheral::add_ioreg(reg_addr_t addr, uint8_t mask, bool readonly)
{
    regbit_t rb = regbit_t(addr, 0, mask);
    m_device->add_ioreg_handler(rb, *this, readonly);
}

uint8_t Peripheral::read_ioreg(reg_addr_t addr) const
{
    return m_device->core().ioctl_read_ioreg(addr);
}

void Peripheral::write_ioreg(const regbit_t& rb, uint8_t value)
{
    m_device->core().ioctl_write_ioreg(rb, value);
}

uint64_t Peripheral::read_ioreg(const regbit_compound_t& rbc) const
{
    uint64_t v = 0;
    for (size_t i = 0; i < rbc.size(); ++i)
        v |= rbc.compound(read_ioreg(rbc[i].addr), i);
    return v;
}

void Peripheral::write_ioreg(const regbit_compound_t& rbc, uint64_t value)
{
    for (size_t i = 0; i < rbc.size(); ++i)
        write_ioreg(rbc[i], rbc.extract(value, i));
}

void Peripheral::set_ioreg(const regbit_compound_t& rbc)
{
    for (auto& rb : rbc)
        set_ioreg(rb);
}

void Peripheral::clear_ioreg(const regbit_compound_t& rbc)
{
    for (auto& rb : rbc)
        set_ioreg(rb);
}

/**
   Helper function to register an interrupt vector.
*/
bool Peripheral::register_interrupt(int_vect_t vector, InterruptHandler& handler) const
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

/**
   Helper function to obtain a pointer to a signal from another peripheral.
*/
Signal* Peripheral::get_signal(ctl_id_t ctl_id) const
{
    if (m_device) {
        ctlreq_data_t d;
        bool status = m_device->ctlreq(ctl_id, AVR_CTLREQ_GET_SIGNAL, &d);
        if (status)
            return reinterpret_cast<Signal*>(d.data.as_ptr());
    }
    return nullptr;
}


//=======================================================================================

DummyController::DummyController(ctl_id_t id, const std::vector<dummy_register_t>& regs)
:Peripheral(id)
,m_registers(regs)
{}

bool DummyController::init(Device& device)
{
    bool status = Peripheral::init(device);
    for (dummy_register_t r : m_registers)
        add_ioreg(r.reg);
    return status;
}

void DummyController::reset()
{
    for (dummy_register_t r : m_registers)
        write_ioreg(r.reg, r.reset);
}
