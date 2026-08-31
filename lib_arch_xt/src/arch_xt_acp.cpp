/*
 * arch_xt_acp.cpp
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

#include "arch_xt_acp.h"
#include "arch_xt_dac.h"
#include "avr_io/io_acp.h"
#include "core/sim_sleep.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE

using namespace ACP;


//=======================================================================================

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(AC_t, reg))

#define REG_OFS(reg) \
    offsetof(AC_t, reg)


//Comparator hysteresis values in Volts
//First row is for normal mode, second for low-power mode
const double Hysteresis[2][4] = {
    { 0.0, 0.01, 0.03, 0.06 },
    { 0.0, 0.01, 0.025, 0.05 }
};


ArchXT_ACP::ArchXT_ACP(int num, const ArchXT_ACPConfig& config)
:Peripheral(AVR_IOCTL_ACP(0x30 + num))
,m_config(config)
,m_intflag(false)
,m_curr_pos_input(0)
,m_curr_neg_input(0)
,m_input_hook(*this, &ArchXT_ACP::input_raised)
,m_sleeping(false)
,m_hysteresis(0.0)
{
    m_signal.raise(Signal_Output, 0);
    m_signal.raise(Signal_AcompRefChange, 0.0);
}


bool ArchXT_ACP::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA));
    add_ioreg(REG_ADDR(MUXCTRLA), AC_INVERT_bm | AC_MUXPOS_gm | AC_MUXNEG_gm);
    add_ioreg(REG_ADDR(DACREF), AC_DACREF_gm);
    add_ioreg(REG_ADDR(INTCTRL), AC_CMP_bm);
    add_ioreg(REG_ADDR(STATUS), AC_STATE_bm, IORegister::RO);
    add_ioreg(REG_ADDR(STATUS), AC_CMP_bm, IORegister::Strobe);

    status &= m_intflag.init(device,
                             DEF_REGBIT_B(INTCTRL, AC_CMP),
                             DEF_REGBIT_B(STATUS, AC_CMP),
                             m_config.iv_cmp);

    //Connect to the VREF signal to receive VCC and intref changes, required for calculating
    //the hysteresis and the internal DAC
    Signal* vref_sig = get_signal(AVR_IOCTL_VREF);
    if (!vref_sig) {
        logger().err("No VREF peripheral found.");
        return false;
    }
    vref_sig->connect(m_input_hook, 0);
    m_input_hook.add_filter(0, VREF::Signal_VCCChange);
    m_input_hook.add_filter(0, VREF::Signal_IntRefChange, m_config.vref_channel);

    status &= register_channels(m_config.pos_channels, vref_sig, true);
    status &= register_channels(m_config.neg_channels, vref_sig, false);

    return status;
}


bool ArchXT_ACP::register_channels(const std::vector<channel_config_t>& channels, Signal* vref_sig, bool pol)
{
    //Connect to the signal for each channel. The hook tag is the channel config index and goes this way:
    //Positive side channels : indexes 1 .. N
    //Negative side channels : indexes -1 .. -N
    for (int ix = 0; ix < (int) channels.size(); ++ix) {
        auto& channel = channels[ix];
        int tag = pol ? (ix + 1) : (-ix - 1);
        switch(channel.type) {
            case Channel_Pin: {
                Pin* pin = device()->find_pin(channel.pin);
                if (!pin) {
                    logger().err("Pin %s not found.", channel.pin.str().c_str());
                    return false;
                }
                pin->signal().connect(m_input_hook, tag);
                m_input_hook.add_filter(tag, Pin::Signal_VoltageChange);
            } break;

            //case Channel_IntRef: nothing to do

            case Channel_DAC: {
                Signal* s = get_signal(AVR_IOCTL_DAC(channel.per_num));
                if (!s) {
                    logger().err("No DAC peripheral found.");
                    return false;
                }
                s->connect(m_input_hook, tag);
                m_input_hook.add_filter(tag, ArchXT_DAC::Signal_Output);
            } break;

            default: break;
        }
    }

    return true;
}


void ArchXT_ACP::reset(int)
{
    m_sleeping = false;
    m_curr_pos_input = 0;
    m_curr_neg_input = 0;
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
        data->data = m_signal.data(Signal_AcompRefChange);
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
        uint8_t pos_ch_regval = DEF_BITSPEC_F(AC_MUXPOS).extract(data.value);
        m_curr_pos_input = find_reg_config<channel_config_t>(m_config.pos_channels, pos_ch_regval);
        if (m_curr_pos_input == -1) {
            device()->crash(CRASH_BAD_CTL_IO, "ACP: Invalid positive channel configuration");
            return;
        }

        //Update the selection for the negative input
        uint8_t neg_ch_regval = DEF_BITSPEC_F(AC_MUXNEG).extract(data.value);
        m_curr_neg_input = find_reg_config<channel_config_t>(m_config.neg_channels, neg_ch_regval);
        if (m_curr_neg_input == -1) {
            device()->crash(CRASH_BAD_CTL_IO, "ACP: Invalid negative channel configuration");
            return;
        }

        update_output();
    }

    else if (reg_ofs == REG_OFS(DACREF)) {
        update_DAC();
        update_output();
    }
}

/*
* Update the DAC value and raise the corresponding signal
*/
void ArchXT_ACP::update_DAC()
{
    vardata_t vref = m_input_hook.data(0, VREF::Signal_IntRefChange, m_config.vref_channel);
    double dac_value = vref.as_double() * READ_IOREG(DACREF) / 256.0;
    m_signal.raise(Signal_AcompRefChange, dac_value);
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
    vardata_t vcc = m_input_hook.data(0, VREF::Source_VCC);
    m_hysteresis = hyst_volt / vcc.as_double();
}


double ArchXT_ACP::read_channel(bool polarity)
{
    int index = polarity ? m_curr_pos_input : m_curr_neg_input;
    int tag = polarity ? (index + 1) : (-index - 1);
    auto& channel = polarity ? m_config.pos_channels[index] : m_config.neg_channels[index];
    switch(channel.type) {
        case Channel_Pin:
            return m_input_hook.data(tag, Pin::Signal_VoltageChange).as_double();

        case Channel_AcompRef:
            return m_signal.data(Signal_AcompRefChange).as_double();

        case Channel_IntRef:
            return m_input_hook.data(0, VREF::Signal_IntRefChange, m_config.vref_channel).as_double();

        case Channel_DAC:
            return m_input_hook.data(tag, ArchXT_DAC::Signal_Output).as_double();

        default:
            return 0.0;
    }
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
        double pos = read_channel(true);
        double neg = read_channel(false);

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
void ArchXT_ACP::input_raised(const signal_data_t& sigdata, int hooktag)
{
    //If the change comes from VREF
    if (hooktag == 0) {
        if (sigdata.sigid == VREF::Signal_VCCChange)
            update_hysteresis();
        else //IntRefChange
            update_DAC();
        update_output();
    }

    else if ((hooktag > 0 && (hooktag - 1) == m_curr_pos_input) ||
             (hooktag < 0 && (-hooktag - 1) == m_curr_neg_input)) {

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
