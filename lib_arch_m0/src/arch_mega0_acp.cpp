/*
 * arch_mega0_acp.cpp
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

#include "arch_mega0_acp.h"
#include "arch_mega0_io.h"
#include "arch_mega0_io_utils.h"
#include "core/sim_sleep.h"
#include "core/sim_device.h"


//=======================================================================================

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(AC_t, reg))

#define REG_OFS(reg) \
    offsetof(AC_t, reg)

typedef AVR_ArchMega0_ACP_Config cfg_t;


enum HookTag {
    HookTag_VREF,
    HookTag_PosMux,
    HookTag_NegMux,
};

AVR_ArchMega0_ACP::AVR_ArchMega0_ACP(int num, const cfg_t& config)
:AVR_Peripheral(AVR_IOCTL_ACP(0x30 + num))
,m_config(config)
,m_intflag(false)
,m_vref_signal(nullptr)
,m_sleeping(false)
{
    m_signal.set_data(Signal_Output, 0);
    m_signal.set_data(Signal_DAC, 0.0);
}

bool AVR_ArchMega0_ACP::init(AVR_Device& device)
{
    bool status = AVR_Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), AC_ENABLE_bm);
    add_ioreg(REG_ADDR(MUXCTRLA), AC_MUXPOS_gm | AC_MUXNEG_gm);
    add_ioreg(REG_ADDR(DACREF), AC_DATA_gm);
    add_ioreg(REG_ADDR(INTCTRL), AC_CMP_bm);
    add_ioreg(REG_ADDR(STATUS), AC_STATE_bm, true);
    add_ioreg(REG_ADDR(STATUS), AC_CMP_bm);

    status &= m_intflag.init(device,
                             DEF_REGBIT_B(INTCTRL, AC_CMP),
                             DEF_REGBIT_B(STATUS, AC_CMP),
                             m_config.iv_cmp);

    m_vref_signal = dynamic_cast<AVR_DataSignal*>(get_signal(AVR_IOCTL_VREF));
    if (m_vref_signal)
        m_vref_signal->connect_hook(this, HookTag_VREF);
    else
        status = false;

    status &= register_channels(m_pos_mux, m_config.pos_channels);
    status &= register_channels(m_neg_mux, m_config.neg_channels);

    m_pos_mux.signal().connect_hook(this, HookTag_PosMux);
    m_neg_mux.signal().connect_hook(this, HookTag_NegMux);

    return status;
}

bool AVR_ArchMega0_ACP::register_channels(AVR_DataSignalMux& mux, const std::vector<channel_config_t>& channels)
{
    for (auto channel : channels) {
        switch(channel.type) {
            case Channel_Pin: {
                AVR_Pin* pin = device()->find_pin(channel.pin);
                if (pin)
                    mux.add_mux(pin->signal(), AVR_Pin::Signal_AnalogValueChange);
                else
                    return false;
            } break;

            case Channel_AcompRef:
                mux.add_mux();
                break;
        }
    }

    return true;
}

void AVR_ArchMega0_ACP::reset()
{
    m_sleeping = false;
    m_intflag.update_from_ioreg();
    m_pos_mux.set_selection(0);
    m_neg_mux.set_selection(0);
    update_DAC();
    update_output();
}

bool AVR_ArchMega0_ACP::ctlreq(uint16_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }

    else if (req == AVR_CTLREQ_ACP_GET_DAC) {
        data->data = m_signal.data(Signal_DAC);
        return true;
    }

    return false;
}

//I/O register callback reimplementation
void AVR_ArchMega0_ACP::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        if ((data.posedge | data.negedge) & AC_ENABLE_bm)
            update_output();
    }

    else if (reg_ofs == REG_OFS(MUXCTRLA)) {
        //Update the selection for the positive input
        uint8_t pos_ch_regval = DEF_BITMASK_F(AC_MUXPOS).extract(data.value);
        int pos_ch_ix = find_reg_config<channel_config_t>(m_config.pos_channels, pos_ch_regval);
        if (pos_ch_ix == -1) {
            device()->crash(CRASH_BAD_CTL_IO, "ACP: Invalid positive channel configuration");
            return;
        }
        m_pos_mux.set_selection(pos_ch_ix);

        //Update the selection for the negative input
        uint8_t neg_ch_regval = DEF_BITMASK_F(AC_MUXNEG).extract(data.value);
        int neg_ch_ix = find_reg_config<channel_config_t>(m_config.neg_channels, neg_ch_regval);
        if (neg_ch_ix == -1) {
            device()->crash(CRASH_BAD_CTL_IO, "ACP: Invalid negative channel configuration");
            return;
        }
        m_neg_mux.set_selection(neg_ch_ix);
    }

    else if (reg_ofs == REG_OFS(DACREF)) {
        update_DAC();
        update_output();
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_intflag.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(STATUS)) {
        //If we're writing a 1 to the interrupt flag bit, it clears the bit and cancels the interrupt
        if (data.value & AC_CMP_bm)
            m_intflag.clear_flag();
    }
}

/*
* Update the DAC value and raise the corresponding signal
*/
void AVR_ArchMega0_ACP::update_DAC()
{
    vardata_t vref = m_vref_signal->data(AVR_IO_VREF::Signal_IntRefChange, m_config.vref_channel);
    double dac_value = vref.as_double() * READ_IOREG(DACREF) / 256.0;
    m_signal.raise_d(Signal_DAC, dac_value);
}

void AVR_ArchMega0_ACP::update_output()
{
    logger().dbg("Updating output");

    //if the device is paused by a sleep mode, no further processing
    if (m_sleeping) return;

    //Compute the new output state
    bool enabled = TEST_IOREG(CTRLA, AC_ENABLE);
    bool old_state = TEST_IOREG(STATUS, AC_STATE);
    bool new_state;

    if (enabled) {
        double pos, neg;

        if (m_pos_mux.connected())
            pos = m_pos_mux.signal().data(0).as_double();
        else
            pos = m_signal.data(Signal_DAC).as_double();

        if (m_neg_mux.connected())
            neg = m_neg_mux.signal().data(0).as_double();
        else
            neg = m_signal.data(Signal_DAC).as_double();

        new_state = (pos > neg);

        logger().dbg("Comparison: p=%g, n=%g, state=%d, old=%d",
                     pos, neg,
                     new_state ? 1 : 0,
                     old_state ? 1 : 0);

        //If the state has changed, raise the interrupt (if enabled) and the signal
        if (new_state ^ old_state) {
            if (enabled)
                m_intflag.set_flag();
            m_signal.raise_u(Signal_Output, new_state);
        }

    } else {

        new_state = false;

    }

    //Update the state in the register
    WRITE_IOREG_B(STATUS, AC_STATE, new_state);
}

/*
* Callback from the pin signal hook.
*/
void AVR_ArchMega0_ACP::raised(const signal_data_t& sigdata, uint16_t hooktag)
{
    if (hooktag == HookTag_VREF) {
        if (sigdata.sigid == AVR_IO_VREF::Signal_IntRefChange && sigdata.index == m_config.vref_channel) {
            update_DAC();
            update_output();
        }
    }
    else if (hooktag == HookTag_PosMux) {
        update_output();
    }
    else if (hooktag == HookTag_NegMux) {
        update_output();
    }
}

/*
* Sleep management
*/
void AVR_ArchMega0_ACP::sleep(bool on, AVR_SleepMode mode)
{
    if (mode > AVR_SleepMode::Standby || (mode == AVR_SleepMode::Standby && !TEST_IOREG(CTRLA, AC_RUNSTDBY)))
        m_sleeping = on;
}
