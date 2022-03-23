/*
 * sim_interrupt.cpp
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

#include "sim_interrupt.h"
#include "sim_core.h"
#include "sim_device.h"


//========================================================================================

AVR_InterruptController::AVR_Interrupt::AVR_Interrupt()
:state(IntrState_Unused)
,handler(nullptr)
{}


//========================================================================================

AVR_InterruptController::AVR_InterruptController(unsigned int size)
:AVR_Peripheral(AVR_IOCTL_INTR)
,m_interrupts(size)
,m_irq_vector(AVR_INTERRUPT_NONE)
{
	m_interrupts[0].state = IntrState_Idle; //The reset vector is always available
}

void AVR_InterruptController::reset()
{
	//Reset the state of all vectors
	for (uint8_t i = 0; i < m_interrupts.size(); i++) {
		if (m_interrupts[i].state != IntrState_Unused)
			m_interrupts[i].state = IntrState_Idle;
	}

	m_irq_vector = AVR_INTERRUPT_NONE;
}

bool AVR_InterruptController::ctlreq(uint16_t req, ctlreq_data_t* data)
{
	if (req == AVR_CTLREQ_GET_SIGNAL) {
		data->data = &m_signal;
		return true;
	}
	else if (req == AVR_CTLREQ_INTR_REGISTER) {
		unsigned int vector = data->index;

		if (vector == 0) {
			ERROR_LOG(device()->logger(), "Attempt to register reset vector", "");
		}
		else if (vector >= m_interrupts.size()) {
			ERROR_LOG(device()->logger(), "Invalid interrupt vector %d", vector);
		}
		else if (m_interrupts[vector].state != IntrState_Unused) {
			ERROR_LOG(device()->logger(), "Double registration on vector %d", vector);
		}
		else {
			m_interrupts[vector].state = IntrState_Idle;
			AVR_InterruptHandler* t = reinterpret_cast<AVR_InterruptHandler*>(data->data.as_ptr());
			t->m_intctl = this;
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
void AVR_InterruptController::sleep(bool on, AVR_SleepMode mode)
{
	if (!on) return;

	for (size_t v = 0; v < m_interrupts.size(); ++v) {
		if (m_interrupts[v].state == IntrState_Raised) {
			m_signal.raise_u(0, v, IntrState_Raised);
		}
	}
}

void AVR_InterruptController::cpu_ack_irq()
{
	cpu_ack_irq(m_irq_vector);
	update_irq();
}

void AVR_InterruptController::cpu_ack_irq(int_vect_t vector)
{
	m_interrupts[vector].state = IntrState_Idle;

	if (m_interrupts[vector].handler)
		m_interrupts[vector].handler->interrupt_ack_handler(vector);
}

void AVR_InterruptController::cpu_reti()
{
	update_irq();
}

void AVR_InterruptController::update_irq()
{
	m_irq_vector = get_next_irq();
}

void AVR_InterruptController::set_interrupt_state(int_vect_t vector, AVR_InterruptState new_state)
{
	m_interrupts[vector].state = new_state;
}

void AVR_InterruptController::raise_interrupt(int_vect_t vector)
{
	//If the interrupt is unused or already raised, no op
	if (m_interrupts[vector].state != IntrState_Idle) return;

	m_interrupts[vector].state = IntrState_Raised;

	m_signal.raise_u(0, vector, IntrState_Raised);

	update_irq();
}

void AVR_InterruptController::cancel_interrupt(int_vect_t vector)
{
	if (m_interrupts[vector].state == IntrState_Raised) {
		m_interrupts[vector].state = IntrState_Idle;
		if (m_irq_vector == vector)
			update_irq();
	}
}

void AVR_InterruptController::disconnect_handler(AVR_InterruptHandler* handler)
{
	for (size_t v = 0; v < m_interrupts.size(); v++) {
		if (m_interrupts[v].handler == handler) {
			m_interrupts[v].state = IntrState_Unused;
			m_interrupts[v].handler->m_intctl = nullptr;
			m_interrupts[v].handler = nullptr;
		}
	}

	if (device()->state() < AVR_Device::State_Destroying)
		update_irq();
}


//========================================================================================

AVR_InterruptHandler::AVR_InterruptHandler()
:m_intctl(nullptr)
{}

AVR_InterruptHandler::~AVR_InterruptHandler()
{
	if (m_intctl)
		m_intctl->disconnect_handler(this);
}

void AVR_InterruptHandler::raise_interrupt(int_vect_t vector) const
{
	if (m_intctl)
		m_intctl->raise_interrupt(vector);
}

void AVR_InterruptHandler::cancel_interrupt(int_vect_t vector) const
{
	if (m_intctl)
		m_intctl->cancel_interrupt(vector);
}

void AVR_InterruptHandler::interrupt_ack_handler(int_vect_t vector)
{}

//=============================================================================

AVR_InterruptFlag::AVR_InterruptFlag(bool clear_on_ack)
:m_clr_on_ack(clear_on_ack)
,m_vector(AVR_INTERRUPT_NONE)
,m_raised(false)
,m_flag_reg(nullptr)
,m_enable_reg(nullptr)
{}

bool AVR_InterruptFlag::init(AVR_Device& device,
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
		ctlreq_data_t d = { .data = this, .index = m_vector };
		vector_ok = device.ctlreq(AVR_IOCTL_INTR, AVR_CTLREQ_INTR_REGISTER, &d);
	}
	else if (vector < 0) {
		vector_ok = true;
	}
	else {
		ERROR_LOG(device.logger(), "Interrupt flag init with reset vector", "");
		vector_ok = false;
	}
	
	return m_flag_reg && m_enable_reg && vector_ok;
}

int AVR_InterruptFlag::update_from_ioreg()
{
	bool enable = m_rb_enable.extract(m_enable_reg->value());
	bool flag = m_rb_flag.extract(m_flag_reg->value());
	if (m_raised) {
		if (!enable || !flag) {
			cancel_interrupt(m_vector);
			m_raised = false;
			return -1;
		} else {
			return 0;
		}
	} else {
		if (enable && flag) {
			raise_interrupt(m_vector);
			m_raised = true;
			return 1;
		} else {
			return 0;
		}
	}
}

bool AVR_InterruptFlag::set_flag(uint8_t mask)
{
	uint8_t new_flag_reg = m_rb_flag.set_to(m_flag_reg->value(), mask);
	m_flag_reg->set(new_flag_reg);
	bool flag_is_raised = m_rb_flag.extract(new_flag_reg);
	if (!m_raised && flag_is_raised && m_rb_enable.extract(m_enable_reg->value())) {
		raise_interrupt(m_vector);
		m_raised = true;
		return true;
	} else {
		return false;
	}
}

bool AVR_InterruptFlag::clear_flag(uint8_t mask)
{
	uint8_t new_flag_reg = m_rb_flag.clear_from(m_flag_reg->value(), mask);
	m_flag_reg->set(new_flag_reg);
	bool flag_is_raised = m_rb_flag.extract(new_flag_reg);
	if (m_raised && !flag_is_raised) {
		cancel_interrupt(m_vector);
		m_raised = false;
		return true;
	} else {
		return false;
	}
}

//bool AVR_InterruptFlag::trigger(uint8_t mask)
//{
//	//Boolean set if the flag was already raised before this function call
//	bool was_set = m_rb_flag.extract(m_flag_reg->value());
//	//Set the register flags as per the mask in argument
//	m_flag_reg->set(m_rb_flag.set_to(m_flag_reg->value(), mask));
//	//The trigger condition is if the flag field goes from zero to non-zero
//	bool triggered = m_rb_flag.extract(m_flag_reg->value()) && !was_set;
//	//if triggered and the interrupt is enabled, raise the interrupt
//	if (triggered && m_rb_enable.extract(m_enable_reg->value()))
//		raise_interrupt(m_vector);
//
//	return triggered;
//}

//bool AVR_InterruptFlag::trigger_if_set(uint8_t mask)
//{
//	//Boolean set if the flag is set
//	bool is_set = m_rb_flag.extract(m_flag_reg->value()) & mask;
//	bool triggered = is_set && m_rb_enable.extract(m_enable_reg->value());
//	//if set, raise the interrupt
//	if (triggered)
//		raise_interrupt(m_vector);
//	return triggered;
//}

//bool AVR_InterruptFlag::cancel(uint8_t mask)
//{
//	//Boolean set if the flag was raised before this function call
//	bool was_set = m_rb_flag.extract(m_flag_reg->value());
//	//Clear the flag bit(s) as per the mask in argument
//	m_flag_reg->set(m_rb_flag.clear_from(m_flag_reg->value(), mask));
//	//Boolean set if the flag field goes from non-zero to zero
//	bool cleared = !m_rb_flag.extract(m_flag_reg->value()) && was_set;
//	//If cleared, cancel the interrupt
//	if (cleared && m_rb_enable.extract(m_enable_reg->value()))
//		cancel_interrupt(m_vector);
//
//	return cleared;
//}

//bool AVR_InterruptFlag::cancel_if_clear(uint8_t mask)
//{
//	bool is_clear = !(m_rb_flag.extract(m_flag_reg->value()) & mask);
//	bool canceled = is_clear && m_rb_enable.extract(m_enable_reg->value());
//	if (canceled)
//		cancel_interrupt(m_vector);
//	return canceled;
//}

void AVR_InterruptFlag::interrupt_ack_handler(int_vect_t vector)
{
	//If the clear-on-ack is enabled, clear the flag field and
	//cancel the interrupt
	if (m_clr_on_ack) {
		m_flag_reg->set(m_rb_flag.clear_from(m_flag_reg->value()));
		cancel_interrupt(m_vector);
		m_raised = false;
	}
}
