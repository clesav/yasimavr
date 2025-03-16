/*
 * arch_avr_extint.cpp
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_avr_extint.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

enum class ExtIntMode {
    Low,
    Toggle,
    Falling,
    Rising
};


//=======================================================================================
/*
 * Constructor of a ExtInt controller
 */
ArchAVR_ExtInt::ArchAVR_ExtInt(const ArchAVR_ExtIntConfig& config)
:Peripheral(AVR_IOCTL_EXTINT)
,m_config(config)
,m_extint_pin_value(0)
,m_pcint_pin_value(config.pc_ints.size(), 0)
,m_pin_hook(*this, &ArchAVR_ExtInt::pin_signal_raised)
{}

/*
 * Initialisation of a ExtInt controller
 */
bool ArchAVR_ExtInt::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Defines all the I/O registers
    add_ioreg(m_config.rb_extint_ctrl);
    add_ioreg(m_config.rb_extint_mask);
    add_ioreg(m_config.rb_extint_flag);
    add_ioreg(m_config.rb_pcint_ctrl);
    add_ioreg(m_config.rb_pcint_flag);

    for (auto& pc_int : m_config.pc_ints)
        add_ioreg(pc_int.reg_mask);

    //Register all the EXTINT interrupts and connect to the pin signal
    for (unsigned int i = 0; i < m_config.ext_ints.size(); ++i) {
        auto& ext_int = m_config.ext_ints[i];

        status &= register_interrupt(ext_int.vector, *this);

        Pin* pin = device.find_pin(ext_int.pin);
        if (pin)
            pin->signal().connect(m_pin_hook, i);
    }

    //Register all the Pin Change registers, interrupts and pins
    for (unsigned int i = 0; i < m_config.pc_ints.size(); ++i) {
        auto& pc_int = m_config.pc_ints[i];

        add_ioreg(pc_int.reg_mask);

        status &= register_interrupt(pc_int.vector, *this);

        for (int j = 0; j < 8; ++j) {
            Pin* pin = device.find_pin(pc_int.pins[j]);
            if (pin)
                pin->signal().connect(m_pin_hook, 0x100 + i * 8 + j);
        }
    }

    return status;
}


void ArchAVR_ExtInt::reset()
{
    m_extint_pin_value = 0;
    std::fill(m_pcint_pin_value.begin(), m_pcint_pin_value.end(), 0);
}


bool ArchAVR_ExtInt::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    return false;
}


void ArchAVR_ExtInt::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //If we're writing a 1 to a interrupt flag bit, it cancels the corresponding interrupt if it
    //has not been executed yet
    if (addr == m_config.rb_extint_flag.addr) {
        uint8_t value = m_config.rb_extint_flag.extract(data.value);
        for (unsigned int i = 0; i < m_config.ext_ints.size(); ++i) {
            //For an extint pin, we need to check if the trigger condition is still present
            //If it isn't we can cancel the interrupt.
            //The condition is (mode == LOW_LEVEL) and (last pin level is LOW) and (interrupt enabled)
            if (BITSET(value, i)) {
                ExtIntMode pinmode = (ExtIntMode) get_extint_mode(i);
                bool lvl = BITSET(m_extint_pin_value, i);
                bool enabled = test_ioreg(m_config.rb_extint_mask, i);
                if (pinmode != ExtIntMode::Low || lvl || !enabled)
                    cancel_interrupt(m_config.ext_ints[i].vector);
            }
        }
    }
    //For Pin Change interrupt flag, no particular condition to check for clearing
    else if (addr == m_config.rb_pcint_flag.addr) {
        for (unsigned int i = 0; i < m_config.pc_ints.size(); ++i) {
            if (BITSET(data.value, i))
                cancel_interrupt(m_config.pc_ints[i].vector);
        }
    }
}


void ArchAVR_ExtInt::interrupt_ack_handler(int_vect_t vector)
{
    //First, iterate over the extint vector to find the one just acked.
    //we need to check if the trigger condition is still present
    //It is is, we must re-raise the interrupt. If not, we clear the flag.
    //The condition is (mode == LOW_LEVEL) and (last pin level is LOW) and (interrupt enabled)
    for (unsigned int i = 0; i < m_config.ext_ints.size(); ++i) {
        if (vector == m_config.ext_ints[i].vector) {
            //For an extint pin,
            ExtIntMode pinmode = (ExtIntMode) get_extint_mode(i);
            bool lvl = BITSET(m_extint_pin_value, i);
            bool enabled = test_ioreg(m_config.rb_extint_mask, i);
            if (pinmode != ExtIntMode::Low || lvl || !enabled) {
                clear_ioreg(m_config.rb_extint_flag, i);
            } else {
                raise_interrupt(vector);
            }
            return;
        }
    }

    //If the vector is a Pin Change interrupt, we clear the interrupt flag and return
    for (unsigned int i = 0; i < m_config.pc_ints.size(); ++i) {
        if (vector == m_config.pc_ints[i].vector) {
            clear_ioreg(m_config.rb_pcint_flag, i);
            return;
        }
    }
}


void ArchAVR_ExtInt::pin_signal_raised(const signal_data_t& sigdata, int hooktag)
{
    if (sigdata.sigid != Pin::Signal_DigitalChange) return;

    bool pin_level = sigdata.data.as_uint();
    uint8_t pin_num = hooktag & 0x00FF;
    bool is_pc = (hooktag & 0x0100);

    if (is_pc) {
        uint8_t bank = pin_num / 8;
        uint8_t bit = pin_num % 8;

        //Get the old level of this extint
        bool old_level = BITSET(m_pcint_pin_value[bank], bit);
        //Check if this pcint pin is masked or enabled
        bool enabled = test_ioreg(m_config.pc_ints[bank].reg_mask, bit);

        //Raise the interrupt if it's enabled and the level has changed
        if (enabled && (pin_level ^ old_level)) {
            uint8_t v = (old_level ? 2 : 0) | (pin_level ? 1 : 0);
            set_ioreg(m_config.rb_pcint_flag, bank);
            raise_interrupt(m_config.pc_ints[bank].vector);
            m_signal.raise(Signal_PinChange, v, pin_num);
        }

        //Stores the new pin level for the next change
        if (pin_level)
            m_pcint_pin_value[bank] |= 1 << bit;
        else
            m_pcint_pin_value[bank] &= ~(1 << bit);

    } else {

        //Get the old level of this extint
        bool old_level = BITSET(m_extint_pin_value, pin_num);

        //Read the control register for the type of sensing set for this extint
        ExtIntMode mode = (ExtIntMode) get_extint_mode(pin_num);

        //Applies the sensing and determine if the interrupt must be raised
        bool trigger = ((mode == ExtIntMode::Low && !pin_level) ||
                        (mode == ExtIntMode::Toggle && (pin_level ^ old_level)) ||
                        (mode == ExtIntMode::Falling && !pin_level && old_level) ||
                        (mode == ExtIntMode::Rising && pin_level && !old_level));

        //Raise the interrupt if it's enabled and set the corresponding bit in the flag register
        if (trigger && test_ioreg(m_config.rb_extint_mask, pin_num)) {
            uint8_t v = (old_level ? 2 : 0) | (pin_level ? 1 : 0);
            set_ioreg(m_config.rb_extint_flag, pin_num);
            raise_interrupt(m_config.ext_ints[pin_num].vector);
            m_signal.raise(Signal_ExtInt, v, pin_num);
        }

        //Stores the new pin level for the next change
        if (pin_level)
            m_extint_pin_value |= 1 << pin_num;
        else
            m_extint_pin_value &= ~(1 << pin_num);

    }
}

/*
 * Utility function that returns the sensing mode for a pin
 */
uint8_t ArchAVR_ExtInt::get_extint_mode(uint8_t pin) const
{
    uint8_t rb_value = read_ioreg(m_config.rb_extint_ctrl);
    uint8_t mode = (rb_value >> (pin << 1)) & 0x03;
    return mode;
}
