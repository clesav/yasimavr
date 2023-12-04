/*
 * arch_xt_acp.cpp
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

#include "arch_xt_acp.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_sleep.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE

//=======================================================================================

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(AC_t, reg))

#define REG_OFS(reg) \
    offsetof(AC_t, reg)

typedef ArchXT_ACPConfig cfg_t;


enum HookTag {
    HookTag_VREF,
    HookTag_PosMux,
    HookTag_NegMux,
};


//Comparator hysteresis values in Volts
//First row is for normal mode, second for low-power mode
const double Hysteresis[2][4] = {
    { 0.0, 0.01, 0.03, 0.06 },
    { 0.0, 0.01, 0.025, 0.05 }
};


ArchXT_ACP::ArchXT_ACP(int num, const cfg_t& config)
:Peripheral(AVR_IOCTL_ACP(0x30 + num))
,m_config(config)
,m_intflag(false)
,m_vref_signal(nullptr)
,m_sleeping(false)
,m_hysteresis(0.0)
{
    m_signal.set_data(Signal_Output, 0);
    m_signal.set_data(Signal_DAC, 0.0);
}

bool ArchXT_ACP::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA));
    add_ioreg(REG_ADDR(MUXCTRLA), AC_INVERT_bm | AC_MUXPOS_gm | AC_MUXNEG_gm);
    add_ioreg(REG_ADDR(DACREF), AC_DATA_gm);
    add_ioreg(REG_ADDR(INTCTRL), AC_CMP_bm);
    add_ioreg(REG_ADDR(STATUS), AC_STATE_bm, true);
    add_ioreg(REG_ADDR(STATUS), AC_CMP_bm);

    status &= m_intflag.init(device,
                             DEF_REGBIT_B(INTCTRL, AC_CMP),
                             DEF_REGBIT_B(STATUS, AC_CMP),
                             m_config.iv_cmp);

    m_vref_signal = dynamic_cast<DataSignal*>(get_signal(AVR_IOCTL_VREF));
    if (m_vref_signal)
        m_vref_signal->connect(*this, HookTag_VREF);
    else
        status = false;

    status &= register_channels(m_pos_mux, m_config.pos_channels);
    status &= register_channels(m_neg_mux, m_config.neg_channels);

    m_pos_mux.signal().connect(*this, HookTag_PosMux);
    m_neg_mux.signal().connect(*this, HookTag_NegMux);

    return status;
}

bool ArchXT_ACP::register_channels(DataSignalMux& mux, const std::vector<channel_config_t>& channels)
{
    for (auto channel : channels) {
        switch(channel.type) {
            case Channel_Pin: {
                Pin* pin = device()->find_pin(channel.pin);
                if (pin)
                    mux.add_mux(pin->signal(), Pin::Signal_VoltageChange);
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

void ArchXT_ACP::reset()
{
    m_sleeping = false;
    m_intflag.update_from_ioreg();
    m_pos_mux.set_selection(0);
    m_neg_mux.set_selection(0);
    update_DAC();
    update_hysteresis();
    update_output();
}

bool ArchXT_ACP::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
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
void ArchXT_ACP::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        update_hysteresis();
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
void ArchXT_ACP::update_DAC()
{
    vardata_t vref = m_vref_signal->data(VREF::Signal_IntRefChange, m_config.vref_channel);
    double dac_value = vref.as_double() * READ_IOREG(DACREF) / 256.0;
    m_signal.raise(Signal_DAC, dac_value);
}


void ArchXT_ACP::update_hysteresis()
{
    if (!TEST_IOREG(CTRLA, AC_ENABLE))
        return;

    //Obtain the correct absolute value for the hysteresis
    //based on register configuration
    uint8_t lp_mode_sel = READ_IOREG_B(CTRLA, AC_LPMODE);
    uint8_t hyst_mode_sel = READ_IOREG_F(CTRLA, AC_HYSMODE);
    double hyst_volt = Hysteresis[lp_mode_sel][hyst_mode_sel];

    //Convert to a value relative to VCC and store the value
    vardata_t vcc = m_vref_signal->data(VREF::Source_VCC);
    if (vcc.as_double())
        m_hysteresis = hyst_volt / vcc.as_double();
    else
        device()->crash(CRASH_BAD_CTL_IO, "ACP: Invalid VCC value");
}

void ArchXT_ACP::update_output()
{
    logger().dbg("Updating output");

    //if the device is paused by a sleep mode, no further processing
    if (m_sleeping) return;

    //Compute the new output state
    bool enabled = TEST_IOREG(CTRLA, AC_ENABLE);
    uint8_t old_state = READ_IOREG_B(STATUS, AC_STATE);
    uint8_t new_state;

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

        //Determine the new state by applying the hysteresis
        if (old_state && ((pos - neg) < -m_hysteresis))
            new_state = 0;
        else if (!old_state && ((pos - neg) > m_hysteresis))
            new_state = 1;
        else
            new_state = old_state;

        //Invert the output value if enabled
        if (TEST_IOREG(MUXCTRLA, AC_INVERT))
            new_state ^= 1;

        logger().dbg("Comparison: p=%g, n=%g, state=%hhu, old=%hhu", pos, neg, new_state, old_state);

        //If the state has changed, raise the interrupt (if enabled) and the signal
        uint8_t int_mode_sel = READ_IOREG_F(CTRLA, AC_INTMODE);
        bool do_raise;
        if (int_mode_sel == AC_INTMODE_BOTHEDGE_gc)
            do_raise = new_state ^ old_state;
        else if (int_mode_sel == AC_INTMODE_NEGEDGE_gc)
            do_raise = old_state & ~new_state;
        else if (int_mode_sel == AC_INTMODE_POSEDGE_gc)
            do_raise = new_state & ~old_state;
        else
            do_raise = false;

        if (do_raise)
            m_intflag.set_flag();

    } else {

        new_state = 0;

    }

    //Update the state in the register and in the signal
    WRITE_IOREG_B(STATUS, AC_STATE, new_state);
    m_signal.raise(Signal_Output, new_state);
}

/*
* Callback from the pin signal hook.
*/
void ArchXT_ACP::raised(const signal_data_t& sigdata, int hooktag)
{
    if (hooktag == HookTag_VREF) {
        if (sigdata.sigid == VREF::Signal_IntRefChange && sigdata.index == m_config.vref_channel) {
            update_DAC();
            update_output();
        }
        else if (sigdata.sigid == VREF::Signal_VCCChange) {
            update_hysteresis();
            update_output();
        }
    }
    else if (hooktag == HookTag_PosMux || hooktag == HookTag_NegMux) {
        update_output();
    }
}

/*
* Sleep management
*/
void ArchXT_ACP::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !TEST_IOREG(CTRLA, AC_RUNSTDBY)))
        m_sleeping = on;
}
