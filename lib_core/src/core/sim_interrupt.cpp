/*
 * sim_interrupt.cpp
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

#include "sim_interrupt.h"
#include "sim_core.h"
#include "sim_device.h"

YASIMAVR_USING_NAMESPACE


//========================================================================================

InterruptController::interrupt_t::interrupt_t()
:used(false)
,raised(false)
,handler(nullptr)
{}


//========================================================================================

InterruptController::InterruptController(unsigned int size)
:Peripheral(AVR_IOCTL_INTR)
,m_interrupts(size)
,m_irq_vector(AVR_INTERRUPT_NONE)
{
    m_interrupts[0].used = true; //The reset vector is always available
}

void InterruptController::reset()
{
    //Reset the state of all vectors
    for (uint8_t i = 0; i < m_interrupts.size(); i++)
        m_interrupts[i].raised = false;

    m_irq_vector = AVR_INTERRUPT_NONE;
}

bool InterruptController::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_INTR_REGISTER) {
        unsigned int vector = data->index;

        if (vector == 0) {
            logger().err("Attempt to register reset vector");
        }
        else if (vector >= m_interrupts.size()) {
            logger().err("Invalid interrupt vector %d", vector);
        }
        else if (m_interrupts[vector].used) {
            logger().err("Double registration on vector %d", vector);
        }
        else {

            InterruptHandler* t = reinterpret_cast<InterruptHandler*>(data->data.as_ptr());
            if (t) {
                m_interrupts[vector].used = true;
                m_interrupts[vector].handler = t;
                t->m_intctl = this;
            }
        }

        return true;
    }
    else if (req == AVR_CTLREQ_INTR_RAISE) {
        int_vect_t vector = (int_vect_t) data->index;

        if (vector >= 0 && vector < m_interrupts.size()) {
            if (data->data.as_uint()) {
                logger().dbg("Raising vector %d on CTLREQ.", vector);
                raise_interrupt(vector);
            } else {
                logger().dbg("Cancelling vector %d on CTLREQ.", vector);
                cancel_interrupt(vector);
            }
        }

        return true;
    }

    return false;
}

/*
 * If we're entering a sleep mode, we need to re-notify about any pending interrupt
 * that could wake the device at the next cycle.
 * We do that by raising the common signal for any interrupt still in the Raised state.
 */
void InterruptController::sleep(bool on, SleepMode mode)
{
    if (!on) return;

    for (size_t v = 0; v < m_interrupts.size(); ++v) {
        if (m_interrupts[v].raised)
            m_signal.raise_u(Signal_StateChange, State_RaisedFromSleep, v);
    }
}

void InterruptController::cpu_ack_irq()
{
    cpu_ack_irq(m_irq_vector);
    update_irq();
}

void InterruptController::cpu_ack_irq(int_vect_t vector)
{
    m_interrupts[vector].raised = false;

    if (m_interrupts[vector].handler)
        m_interrupts[vector].handler->interrupt_ack_handler(vector);

    m_signal.raise_u(Signal_StateChange, State_Acknowledged, vector);
}

void InterruptController::cpu_reti()
{
    m_signal.raise_u(Signal_StateChange, State_Returned);
    update_irq();
}

void InterruptController::update_irq()
{
    m_irq_vector = get_next_irq();
}

void InterruptController::set_interrupt_raised(int_vect_t vector, bool raised)
{
    m_interrupts[vector].raised = raised;
}

void InterruptController::raise_interrupt(int_vect_t vector)
{
    //If the interrupt is already raised, no op
    if (m_interrupts[vector].used && !m_interrupts[vector].raised) {
        m_interrupts[vector].raised = true;
        m_signal.raise_u(Signal_StateChange, State_Raised, vector);
        update_irq();
    }
}

void InterruptController::cancel_interrupt(int_vect_t vector)
{
    if (m_interrupts[vector].used && m_interrupts[vector].raised) {
        m_interrupts[vector].raised = false;
        m_signal.raise_u(Signal_StateChange, State_Cancelled, vector);
        if (m_irq_vector == vector)
            update_irq();
    }
}

void InterruptController::disconnect_handler(InterruptHandler* handler)
{
    for (size_t v = 0; v < m_interrupts.size(); v++) {
        if (m_interrupts[v].handler == handler) {
            m_interrupts[v].used = false;
            m_interrupts[v].handler->m_intctl = nullptr;
            m_interrupts[v].handler = nullptr;
        }
    }

    if (device()->state() < Device::State_Destroying)
        update_irq();
}


//========================================================================================

InterruptHandler::InterruptHandler()
:m_intctl(nullptr)
{}

InterruptHandler::~InterruptHandler()
{
    if (m_intctl)
        m_intctl->disconnect_handler(this);
}

void InterruptHandler::raise_interrupt(int_vect_t vector) const
{
    if (m_intctl)
        m_intctl->raise_interrupt(vector);
}

void InterruptHandler::cancel_interrupt(int_vect_t vector) const
{
    if (m_intctl)
        m_intctl->cancel_interrupt(vector);
}

void InterruptHandler::interrupt_ack_handler(int_vect_t vector)
{}


//========================================================================================

InterruptFlag::InterruptFlag(bool clear_on_ack)
:m_clr_on_ack(clear_on_ack)
,m_vector(AVR_INTERRUPT_NONE)
,m_raised(false)
,m_flag_reg(nullptr)
,m_enable_reg(nullptr)
{}

bool InterruptFlag::init(Device& device,
                         const regbit_t& rb_enable,
                         const regbit_t& rb_flag,
                         int_vect_t vector)
{
    //Obtain a pointer to the two registers flag and enable
    m_rb_enable = rb_enable;
    m_enable_reg = device.core().get_ioreg(m_rb_enable.addr);

    m_rb_flag = rb_flag;
    m_flag_reg = device.core().get_ioreg(m_rb_flag.addr);

    //Register this as the handler of the interrupt vector with the Interrupt Controller
    m_vector = vector;
    bool vector_ok;
    if (vector > 0) {
        ctlreq_data_t d = { this, m_vector };
        vector_ok = device.ctlreq(AVR_IOCTL_INTR, AVR_CTLREQ_INTR_REGISTER, &d);
    }
    else if (vector < 0) {
        vector_ok = true;
    }
    else {
        device.logger().err("Interrupt flag init with reset vector");
        vector_ok = false;
    }

    return m_flag_reg && m_enable_reg && vector_ok;
}

int InterruptFlag::update_from_ioreg()
{
    bool raised = flag_raised();

    if (m_raised) {
        if (!raised) {
            cancel_interrupt(m_vector);
            m_raised = false;
            return -1;
        } else {
            return 0;
        }
    } else {
        if (raised) {
            raise_interrupt(m_vector);
            m_raised = true;
            return 1;
        } else {
            return 0;
        }
    }
}

bool InterruptFlag::set_flag(uint8_t mask)
{
    uint8_t new_flag_reg = m_rb_flag.set_to(m_flag_reg->value(), mask);
    m_flag_reg->set(new_flag_reg);

    if (!m_raised && flag_raised()) {
        raise_interrupt(m_vector);
        m_raised = true;
        return true;
    } else {
        return false;
    }
}

bool InterruptFlag::clear_flag(uint8_t mask)
{
    uint8_t new_flag_reg = m_rb_flag.clear_from(m_flag_reg->value(), mask);
    m_flag_reg->set(new_flag_reg);

    if (m_raised && !flag_raised()) {
        cancel_interrupt(m_vector);
        m_raised = false;
        return true;
    } else {
        return false;
    }
}

bool InterruptFlag::flag_raised() const
{
    uint8_t en_mask = m_rb_enable.extract(m_enable_reg->value());
    uint8_t fl_mask = m_rb_flag.extract(m_flag_reg->value());
    return !!(en_mask & fl_mask);
}

void InterruptFlag::interrupt_ack_handler(int_vect_t vector)
{
    //If the clear-on-ack is enabled, clear the flag field and
    //cancel the interrupt
    if (m_clr_on_ack) {
        m_flag_reg->set(m_rb_flag.clear_from(m_flag_reg->value()));
        cancel_interrupt(m_vector);
        m_raised = false;
    }
}
