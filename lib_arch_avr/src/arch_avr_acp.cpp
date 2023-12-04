/*
 * arch_avr_acp.cpp
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_avr_acp.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

enum {
    HookTag_Pos = 0,
    HookTag_Neg,

    Mux_PosPin = 0,
    Mux_IntRef,

    Mux_NegPin = 0,
    Mux_NegMuxPins,
};


ArchAVR_ACP::ArchAVR_ACP(int num, const ArchAVR_ACPConfig& config)
:Peripheral(AVR_IOCTL_ACP(0x30 + num))
,m_config(config)
,m_intflag(true)
,m_pos_value(0.0)
,m_neg_value(0.0)
{}


bool ArchAVR_ACP::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rb_disable);
    add_ioreg(m_config.rb_mux_enable);
    add_ioreg(m_config.rb_adc_enable);
    add_ioreg(m_config.rb_bandgap_select);
    add_ioreg(m_config.rb_int_mode);
    add_ioreg(m_config.rb_output, true);
    add_ioreg(m_config.rb_int_enable);
    add_ioreg(m_config.rb_int_flag);

    status &= m_intflag.init(device,
                             m_config.rb_int_enable,
                             m_config.rb_int_flag,
                             m_config.iv_cmp);

    //Find the positive input pin and add it to the positive input mux
    Pin* pos_pin = device.find_pin(m_config.pos_pin);
    if (!pos_pin) {
        logger().err("Positive input pin invalid");
        return false;
    }
    m_pos_mux.add_mux(pos_pin->signal(), Pin::Signal_VoltageChange);

    //Find the negative input pin and add it to the negative input mux
    Pin* neg_pin = device.find_pin(m_config.neg_pin);
    if (!neg_pin) {
        logger().err("Negative input pin invalid");
        return false;
    }
    m_neg_mux.add_mux(neg_pin->signal(), Pin::Signal_VoltageChange);

    //Find the signal from the voltage reference controller and add it to
    //the positive input mux
    DataSignal* vref_sig = dynamic_cast<DataSignal*>(get_signal(AVR_IOCTL_VREF));
    if (!vref_sig) {
        logger().err("No voltage reference signal");
        return false;
    }
    m_pos_mux.add_mux(*vref_sig, VREF::Signal_IntRefChange);

    //Connect the mux pins to the negative input mux
    for (size_t i = 0; i < m_config.mux_pins.size(); ++i) {
        Pin* pin = device.find_pin(m_config.mux_pins[i].pin);
        if (!pin) {
            logger().err("Negative mux pin invalid");
            return false;
        }
        m_neg_mux.add_mux(pin->signal(), Pin::Signal_VoltageChange);
    }

    m_pos_mux.signal().connect(*this, HookTag_Pos);
    m_neg_mux.signal().connect(*this, HookTag_Neg);

    return status;
}


void ArchAVR_ACP::reset()
{
    m_intflag.update_from_ioreg();
    change_pos_channel();
    change_neg_channel();
    m_out_signal.raise(Signal_Output, (unsigned char) 0);
}


bool ArchAVR_ACP::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_out_signal;
        return true;
    }

    return false;
}


//I/O register callback reimplementation

void ArchAVR_ACP::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.rb_disable.addr) {
        clear_ioreg(m_config.rb_output);
    }

    if (addr == m_config.rb_mux_enable.addr ||
        addr == m_config.rb_adc_enable.addr ||
        addr == m_config.rb_mux.addr) {

        change_neg_channel();
        update_state();
    }

    if (addr == m_config.rb_bandgap_select.addr) {
        change_pos_channel();
        update_state();
    }

    if (addr == m_config.rb_int_enable.addr)
        m_intflag.update_from_ioreg();

    //Writing 1 to ACI clears the flag and cancels the interrupt
    if (addr == m_config.rb_int_flag.addr && m_config.rb_int_flag.extract(data.value))
        m_intflag.clear_flag();

}


void ArchAVR_ACP::change_pos_channel()
{
    if (test_ioreg(m_config.rb_bandgap_select))
        m_pos_mux.set_selection(Mux_IntRef);
    else
        m_pos_mux.set_selection(Mux_PosPin);
}


void ArchAVR_ACP::change_neg_channel()
{
    if (test_ioreg(m_config.rb_mux_enable) && !test_ioreg(m_config.rb_adc_enable)) {
        uint8_t mux_regval = read_ioreg(m_config.rb_mux);
        int mux_index = find_reg_config<ArchAVR_ACPConfig::mux_config_t>(m_config.mux_pins, mux_regval);
        if (mux_index < 0) {
            device()->crash(CRASH_BAD_CTL_IO, "ACP: Invalid mux configuration");
            return;
        }
        m_neg_mux.set_selection(Mux_NegMuxPins + mux_index);
    } else {
        m_neg_mux.set_selection(Mux_NegPin);
    }
}


void ArchAVR_ACP::update_state()
{
    logger().dbg("ACP updating");

    if (test_ioreg(m_config.rb_disable)) return;

    bool old_state = test_ioreg(m_config.rb_output);

    bool new_state = (m_pos_value > m_neg_value);

    if (new_state ^ old_state) {
        write_ioreg(m_config.rb_output, new_state);
        m_intflag.set_flag();
        m_out_signal.raise(Signal_Output, (unsigned char) new_state);
    }
}


/*
 * Hook callback, the hooktag determines if it's for the positive or the negative side
 */
void ArchAVR_ACP::raised(const signal_data_t& sigdata, int hooktag)
{
    if (hooktag == HookTag_Pos)
        m_pos_value = sigdata.data.as_double();
    else
        m_neg_value = sigdata.data.as_double();

    update_state();
}
