/*
 * arch_avr_acp.cpp
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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
#include "ioctrl_common/sim_acp.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_USING_NAMESPACE

using namespace ACP;


//=======================================================================================

enum {
    HookTag_PosPin,
    HookTag_NegPin,
    HookTag_IntRef,
    HookTag_NegMuxPins,
};


ArchAVR_ACP::ArchAVR_ACP(int num, const ArchAVR_ACPConfig& config)
:Peripheral(AVR_IOCTL_ACP(0x30 + num))
,m_config(config)
,m_intflag(true)
,m_input_hook(*this, &ArchAVR_ACP::input_raised)
{}


bool ArchAVR_ACP::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rb_disable);
    add_ioreg(m_config.rb_mux_enable);
    add_ioreg(m_config.rb_adc_enable);
    add_ioreg(m_config.rb_bandgap_select);
    add_ioreg(m_config.rb_int_mode);
    add_ioreg(m_config.rb_output, IORegister::RO);
    add_ioreg(m_config.rb_int_enable);
    add_ioreg(m_config.rb_int_flag, IORegister::Strobe);

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
    pos_pin->signal().connect(m_input_hook, HookTag_PosPin);
    m_input_hook.add_filter(HookTag_PosPin, Pin::Signal_VoltageChange);

    //Find the negative input pin and add it to the negative input mux
    Pin* neg_pin = device.find_pin(m_config.neg_pin);
    if (!neg_pin) {
        logger().err("Negative input pin invalid");
        return false;
    }
    pos_pin->signal().connect(m_input_hook, HookTag_NegPin);
    m_input_hook.add_filter(HookTag_NegPin, Pin::Signal_VoltageChange);

    //Connect to the VREF signal to receive VCC and intref changes, required for calculating
    //the hysteresis and the internal DAC
    Signal* vref_sig = get_signal(AVR_IOCTL_VREF);
    if (!vref_sig) {
        logger().err("No VREF peripheral found.");
        return false;
    }
    vref_sig->connect(m_input_hook, HookTag_IntRef);
    m_input_hook.add_filter(HookTag_IntRef, VREF::Signal_IntRefChange, 0);

    //Connect to the signals of the ADC mux
    for (unsigned int i = 0; i < m_config.neg_channels.size(); ++i) {
        auto& channel = m_config.neg_channels[i];
        int tag = HookTag_NegMuxPins + i;
        switch (channel.type) {

            case Channel_Pin: {
                Pin* pin = device.find_pin(channel.pin);
                if (!pin) {
                    logger().err("Pin %s not found.", channel.pin.str().c_str());
                    return false;
                }
                pin->signal().connect(m_input_hook, tag);
                m_input_hook.add_filter(tag, Pin::Signal_VoltageChange);
            } break;

            case Channel_IntRef: {
                vref_sig->connect(m_input_hook, tag);
                m_input_hook.add_filter(tag, VREF::Signal_IntRefChange, 0);
            } break;

            default: break;

        }
    }

    return status;
}


void ArchAVR_ACP::reset(int)
{
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
    if (addr == m_config.rb_disable) {
        clear_ioreg(m_config.rb_output);
    }

    if (addr == m_config.rb_mux_enable ||
        addr == m_config.rb_adc_enable ||
        addr == m_config.rb_mux) {

        update_state();
    }

    if (addr == m_config.rb_bandgap_select) {
        update_state();
    }
}


double ArchAVR_ACP::read_neg_channel() const
{
    if (!test_ioreg(m_config.rb_mux_enable) || test_ioreg(m_config.rb_adc_enable))
        return m_input_hook.data(HookTag_NegPin, Pin::Signal_VoltageChange).as_double();

    int mux_index = find_reg_config<channel_config_t>(m_config.neg_channels, read_ioreg(m_config.rb_mux));
    if (mux_index < 0)
        return 0.0;

    switch(m_config.neg_channels[mux_index].type) {
        case Channel_Pin:
            return m_input_hook.data(HookTag_NegMuxPins + mux_index, Pin::Signal_VoltageChange).as_double();

        case Channel_IntRef:
            return m_input_hook.data(HookTag_IntRef, VREF::Signal_IntRefChange, 0).as_double();

        default:
            return 0.0;
    }
}


void ArchAVR_ACP::update_state()
{
    logger().dbg("ACP updating");

    if (test_ioreg(m_config.rb_disable)) return;

    bool old_state = test_ioreg(m_config.rb_output);

    double pos;
    if (test_ioreg(m_config.rb_bandgap_select))
        pos = m_input_hook.data(HookTag_IntRef, VREF::Signal_IntRefChange, 0).as_double();
    else
        pos = m_input_hook.data(HookTag_PosPin, Pin::Signal_VoltageChange).as_double();

    double neg = read_neg_channel();

    bool new_state = (pos > neg);

    if (new_state ^ old_state) {
        write_ioreg(m_config.rb_output, new_state);
        m_intflag.set_flag();
        m_out_signal.raise(Signal_Output, (unsigned char) new_state);
    }
}


void ArchAVR_ACP::input_raised(const signal_data_t& sigdata, int hooktag)
{
    update_state();
}
