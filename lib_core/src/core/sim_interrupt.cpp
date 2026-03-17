/*
 * sim_interrupt.cpp
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

#include "sim_interrupt.h"
#include "sim_core.h"
#include "sim_device.h"

YASIMAVR_USING_NAMESPACE


//========================================================================================

/**
   Construct the controller with the given vector table size
*/
InterruptController::InterruptController(unsigned int vector_count)
:Peripheral(AVR_IOCTL_INTR)
,m_interrupts(vector_count)
,m_irq(NO_INTERRUPT)
{
    m_interrupts[0].used = true; //The reset vector is always available
}

void InterruptController::reset(int)
{
    //Reset the state of all vectors
    for (unsigned int i = 0; i < m_interrupts.size(); ++i) {
        m_interrupts[i].raised = false;
        m_signal.raise(Signal_StateChange, State_Reset, i);
    }

    m_irq = NO_INTERRUPT;
}

bool InterruptController::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_INTR_REGISTER) {
        int vector = data->index;

        if (vector <= 0) {
            logger().err("Attempt to register an invalid or the reset vector");
        }
        else if ((size_t) vector >= m_interrupts.size()) {
            logger().err("Invalid interrupt vector %d", vector);
        }
        else if (m_interrupts[vector].used) {
            logger().err("Double registration on vector %d", vector);
        }
        else {

            InterruptHandler* t = data->data.as_ptr<InterruptHandler>();
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

        if (vector >= 0 && (size_t) vector < m_interrupts.size()) {
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

    for (unsigned int v = 0; v < m_interrupts.size(); ++v) {
        if (m_interrupts[v].raised)
            m_signal.raise(Signal_StateChange, State_RaisedFromSleep, v);
    }
}

/**
   Used by the CPU to acknowledge the IRQ obtained with cpu_get_irq().
*/
void InterruptController::cpu_ack_irq()
{
    cpu_ack_irq(m_irq.vector);
    update_irq();
}

/**
   Called by the CPU when it acknowledges a vector, i.e. it is about to execute
   the vector address in the flash.
   \n The base implementation calls the ACK handler of the vector and
   raise the signal with the "Acknowledged" state.
*/
void InterruptController::cpu_ack_irq(int_vect_t vector)
{
    m_interrupts[vector].raised = false;

    if (m_interrupts[vector].handler)
        m_interrupts[vector].handler->interrupt_ack_handler(vector);

    m_signal.raise(Signal_StateChange, State_Acknowledged, vector);
}

/**
   Called by the CPU when returning from an ISR (executing a RETI instruction).
   \n The base implementation calls update_irq to determine the next vector to be
   given to the CPU.
*/
void InterruptController::cpu_reti()
{
    m_signal.raise(Signal_StateChange, State_Returned);
    update_irq();
}

/**
   Update the controller, by storing the next vector to be given to the CPU
*/
void InterruptController::update_irq()
{
    m_irq = get_next_irq();
}

///Interrupt state setter
void InterruptController::set_interrupt_raised(int_vect_t vector, bool raised)
{
    m_interrupts[vector].raised = raised;
}

void InterruptController::raise_interrupt(int_vect_t vector)
{
    //If the interrupt is unused or already raised, no op
    if (m_interrupts[vector].used && !m_interrupts[vector].raised) {
        m_interrupts[vector].raised = true;
        m_signal.raise(Signal_StateChange, State_Raised, vector);
        update_irq();
    }
}

void InterruptController::cancel_interrupt(int_vect_t vector)
{
    if (m_interrupts[vector].used && m_interrupts[vector].raised) {
        m_interrupts[vector].raised = false;
        m_signal.raise(Signal_StateChange, State_Cancelled, vector);
        if (m_irq.vector == vector)
            update_irq();
    }
}

void InterruptController::disconnect_handler(InterruptHandler* handler)
{
    for (auto& intr : m_interrupts) {
        if (intr.handler == handler) {
            intr.used = false;
            intr.handler->m_intctl = nullptr;
            intr.handler = nullptr;
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

bool InterruptHandler::interrupt_raised(int_vect_t vector) const
{
    if (m_intctl)
        return m_intctl->interrupt_raised(vector);
    else
        return false;
}

/**
   Callback method called when a vector has been ACK'ed by the CPU.
   (i.e. the CPU is about to jump to the corresponding vector table entry)
   \n The default implementation does nothing.
*/
void InterruptHandler::interrupt_ack_handler(int_vect_t vector)
{}


//========================================================================================

/**
   Construct an Interrupt Flag.

   \param clear_on_ack if true, the flag will be cleared when the interrupt is ACK'ed
   by the CPU. If false, (default) the flag can only be cleared by writing to the register.
*/
AbstractInterruptFlag::AbstractInterruptFlag(bool clear_on_ack)
:m_clr_on_ack(clear_on_ack)
,m_vector(AVR_INTERRUPT_NONE)
{}

/**
   Initialise an Interrupt Flag. Allocates the registers for the flag and the enable
   register fields and register with the interrupt controller for a particular vector.
   \note If the vector is < 0, then no interrupt is registered. (can be used for future
   or unsupported features)
   \note Registering with the reset vector (vector 0) is an error.
   \param rb_enable register location for the flag enable bits
   \param rb_flag register location for the flag state bits
   \param vector interrupt vector index
   \return true if allocations and registrations are successful, false otherwise
*/
bool AbstractInterruptFlag::init(Device& device, int_vect_t vector)
{
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

    return vector_ok;
}

/**
   Update the state of the interrupt flag according to the
   value of the I/O registers.
   This should be called whenever the flag registers are written.
   Allocates the registers for the flag and the enable register fields.
   \return +1 if the interrupt has been raised as a result of the update,
           -1 if it has been cleared, 0 if unchanged.
*/
int AbstractInterruptFlag::update()
{
    if (raised()) {
        if (!flag_raised()) {
            cancel_interrupt(m_vector);
            return -1;
        } else {
            return 0;
        }
    } else {
        if (flag_raised()) {
            raise_interrupt(m_vector);
            return 1;
        } else {
            return 0;
        }
    }
}

void AbstractInterruptFlag::interrupt_ack_handler(int_vect_t vector)
{
    //If the clear-on-ack is enabled, cancel the interrupt
    if (m_clr_on_ack)
        cancel_interrupt(m_vector);
}


//========================================================================================

/**
   Construct an Interrupt Flag.

   \param clear_on_ack if true, the flag will be cleared when the interrupt is ACK'ed
   by the CPU. If false, (default) the flag can only be cleared by writing to the register.
*/
InterruptFlag::InterruptFlag(bool clear_on_ack)
:AbstractInterruptFlag(clear_on_ack)
,m_flag_reg(nullptr)
,m_enable_reg(nullptr)
{}

/**
   Initialise an Interrupt Flag. Allocates the registers for the flag and the enable
   register fields and register with the interrupt controller for a particular vector.
   \note If the vector is < 0, then no interrupt is registered. (can be used for future
   or unsupported features)
   \note Registering with the reset vector (vector 0) is an error.
   \param rb_enable register location for the flag enable bits
   \param rb_flag register location for the flag state bits
   \param vector interrupt vector index
   \return true if allocations and registrations are successful, false otherwise
*/
bool InterruptFlag::init(Device& device,
                         const regmask_t& rm_enable,
                         const regmask_t& rm_flag,
                         int_vect_t vector)
{
    bool ok = AbstractInterruptFlag::init(device, vector);

    //Obtain a pointer to the two registers flag and enable and register this as handler
    //for the two registers
    m_rm_enable = rm_enable;
    m_enable_reg = device.core().get_ioreg(m_rm_enable.addr);
    if (m_enable_reg)
        m_enable_reg->add_handler(*this);

    m_rm_flag = rm_flag;
    m_flag_reg = device.core().get_ioreg(m_rm_flag.addr);
    if (m_flag_reg)
        m_flag_reg->add_handler(*this);

    return m_flag_reg && m_enable_reg && ok;
}

/**
   Set the interrupt flag bits by OR'ing them with the mask argument.
   \return true if the interrupt is raised as a result of the flag bit changes,
   false if the interrupt is unchanged.
*/
bool InterruptFlag::set_flag(bitmask_t mask)
{
    bitmask_t set_mask = m_rm_flag.mask & mask;
    uint8_t new_flag_reg = set_mask.set_from(m_flag_reg->value());
    m_flag_reg->set(new_flag_reg);
    return update() != 0;
}

/**
   Clear the interrupt flag bits by AND'ing them with the mask argument.
   \return true if the interrupt is canceled as a result of the flag bit changes,
   false if the interrupt is unchanged.
*/
bool InterruptFlag::clear_flag(bitmask_t mask)
{
    bitmask_t clear_mask = m_rm_flag.mask & mask;
    uint8_t new_flag_reg = clear_mask.clear_from(m_flag_reg->value());
    m_flag_reg->set(new_flag_reg);
    return update() != 0;
}


bool InterruptFlag::flag_raised() const
{
    uint8_t en_mask = m_enable_reg->value() & m_rm_enable.mask;
    uint8_t fl_mask = m_flag_reg->value() & m_rm_flag.mask;
    return !!(en_mask & fl_mask);
}


void InterruptFlag::interrupt_ack_handler(int_vect_t vector)
{
    AbstractInterruptFlag::interrupt_ack_handler(vector);
    //If the clear-on-ack is enabled, clear the flag field
    if (clear_on_ack())
        m_flag_reg->set(m_rm_flag.mask.clear_from(m_flag_reg->value()));
}


uint8_t InterruptFlag::ioreg_read_handler(reg_addr_t, uint8_t value)
{
    return value;
}


uint8_t InterruptFlag::ioreg_peek_handler(reg_addr_t, uint8_t value)
{
    return value;
}


void InterruptFlag::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    update();
}
