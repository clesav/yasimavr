/*
 * arch_avr_timer.cpp
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

#include "arch_avr_timer.h"


typedef AVR_ArchAVR_TimerConfig CFG;


/*
 * Implementation of a SignalHook for event capture. It just forwards to the main class.
 */
class AVR_ArchAVR_Timer::CaptureHook : public AVR_SignalHook {

public:

    CaptureHook(AVR_ArchAVR_Timer& timer) : m_timer(timer) {}
    virtual ~CaptureHook() {}

    virtual void raised(const signal_data_t& data, uint16_t sigid) override
    {
        m_timer.capt_raised();
    }

private:

    AVR_ArchAVR_Timer& m_timer;

};


/*
 * Implementation of the structure that hold configuration and data for each
 * OutputCompare module
 */
struct AVR_ArchAVR_Timer::OutputCompareChannel {

    const CFG::OC_config_t& config;
    CFG::COM_config_t mode;
    uint8_t reg;
    unsigned char state;
    AVR_InterruptFlag intflag;

    OutputCompareChannel(const CFG::OC_config_t& cfg)
    :config(cfg)
    ,mode({CFG::COM_NoChange, CFG::COM_NoChange, CFG::COM_NoChange, CFG::COM_NoChange })
    ,reg(0)
    ,state(false)
    ,intflag(true)
    {}

    void reset()
    {
        reg = 0;
        state = 0;
        intflag.update_from_ioreg();
    }

};


AVR_ArchAVR_Timer::AVR_ArchAVR_Timer(int num, const CFG& config)
:AVR_Peripheral(AVR_IOCTL_TIMER('_', 0x30 + num))
,m_config(config)
,m_icr(0)
,m_temp(0)
,m_counter(m_timer, (m_config.is_16bits ? 0x10000 : 0x100), m_config.oc_channels.size())
,m_intflag_ovf(true)
,m_intflag_icr(true)
{
    //Calculate the prescaler max value by looking for the highest division factor
    unsigned int maxdiv = 0;
    for (auto clkcfg : m_config.clocks) {
        if (clkcfg.div > maxdiv)
            maxdiv = clkcfg.div;
    }
    m_clk_ps_max = maxdiv;

    m_capt_hook = new CaptureHook(*this);

    for (auto& oc_cfg : m_config.oc_channels) {
        OutputCompareChannel* oc = new OutputCompareChannel(oc_cfg);
        m_oc_channels.push_back(oc);
    }
}


AVR_ArchAVR_Timer::~AVR_ArchAVR_Timer()
{
    delete m_capt_hook;

    for (auto oc : m_oc_channels)
        delete oc;
}


bool AVR_ArchAVR_Timer::init(AVR_Device& device)
{
    bool status = AVR_Peripheral::init(device);

    add_ioreg(m_config.rb_clock.addr);
    add_ioreg(m_config.rbc_mode);
    add_ioreg(m_config.reg_cnt);
    if (m_config.is_16bits)
        add_ioreg(m_config.reg_cnt + 1);

    status &= m_intflag_ovf.init(device,
                                 regbit_t(m_config.reg_int_enable, m_config.vect_ovf.bit),
                                 regbit_t(m_config.reg_int_flag, m_config.vect_ovf.bit),
                                 m_config.vect_ovf.num);
    uint8_t int_bitmask = 1 << m_config.vect_ovf.bit;

    if (m_config.reg_icr.valid()) {
        add_ioreg(m_config.reg_icr);
        if (m_config.is_16bits)
            add_ioreg(m_config.reg_icr + 1);

        status &= m_intflag_icr.init(device,
                                     regbit_t(m_config.reg_int_enable, m_config.vect_icr.bit),
                                     regbit_t(m_config.reg_int_flag, m_config.vect_icr.bit),
                                     m_config.vect_icr.num);
        int_bitmask |= 1 << m_config.vect_icr.bit;
    }

    for (auto oc : m_oc_channels) {
        add_ioreg(oc->config.rb_mode);
        add_ioreg(oc->config.rb_force);
        add_ioreg(oc->config.reg_oc);
        if (m_config.is_16bits)
            add_ioreg(oc->config.reg_oc + 1);

        const CFG::vector_config_t& v = oc->config.vector;
        status &= oc->intflag.init(device,
                                   regbit_t(m_config.reg_int_enable, v.bit),
                                   regbit_t(m_config.reg_int_flag, v.bit),
                                   v.num);
        int_bitmask |= (1 << v.bit);
    }

    add_ioreg(m_config.reg_int_enable, int_bitmask);
    add_ioreg(m_config.reg_int_flag, int_bitmask);

    m_timer.init(*device.cycle_manager(), logger());

    m_counter.set_logger(&logger());
    m_counter.signal().connect_hook(this);

    return status;
}


void AVR_ArchAVR_Timer::reset()
{
    m_temp = 0;
    m_mode = m_config.modes[0];
    m_timer.reset();
    m_timer.set_prescaler(m_clk_ps_max, 1);
    m_counter.reset();
    m_intflag_ovf.update_from_ioreg();

    if (m_config.reg_icr.valid()) {
        m_icr = 0;
        m_intflag_icr.update_from_ioreg();
    }

    for (uint32_t i = 0; i < m_oc_channels.size(); ++i) {
        m_oc_channels[i]->reset();
        m_signal.set_data(Signal_CompOutput, vardata_t(0), i);
    }
}


bool AVR_ArchAVR_Timer::ctlreq(uint16_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_TMR_GET_EXTCLK_HOOK) {
        data->data = &m_counter.ext_tick_hook();
        return true;
    }
    else if (req == AVR_CTLREQ_TMR_GET_CAPT_HOOK) {
        data->data = &m_capt_hook;
        return true;
    }
    return false;
}

uint8_t AVR_ArchAVR_Timer::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    //reading of interrupt flags
    if (addr == m_config.reg_int_flag)
        m_timer.update();

    //8 or 16 bits reading of CNTx
    else if (addr == m_config.reg_cnt) {
        m_timer.update();
        uint16_t v = m_counter.counter();
        value = v & 0x00FF;
        if (m_config.is_16bits)
            m_temp = v >> 8;
    }
    else if (m_config.is_16bits && addr == m_config.reg_cnt + 1) {
        value = m_temp;
    }
    //8 or 16 bits reading of ICR
    else if (addr == m_config.reg_icr) {
        value = m_icr & 0x00FF;
        if (m_config.is_16bits)
            m_temp = m_icr >> 8;
    }
    else if (m_config.is_16bits && addr == m_config.reg_icr + 1) {
        value = m_icr >> 8;
    }

    return value;
}


void AVR_ArchAVR_Timer::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    bool do_reschedule = false;
    bool do_com_reconfig = false;

    //8 or 16 bits writing to CNTx
    if (addr == m_config.reg_cnt) {
        m_counter.set_counter((m_temp << 8) | data.value);
        do_reschedule = true;
    }
    else if (m_config.is_16bits && addr == m_config.reg_cnt + 1) {
        m_temp = data.value;
    }

    //8 or 16 bits writing to ICR
    else if (addr == m_config.reg_icr) {
        m_icr = (m_temp << 8) | data.value;
        if (m_mode.top == CFG::Top_OnIC) {
            update_top();
            do_reschedule = true;
        }
    }
    else if (m_config.is_16bits && addr == m_config.reg_icr + 1) {
        m_temp = data.value;
    }

    else {
        //Check if writing to any registers of each output compare module
        for (uint32_t i = 0; i < m_oc_channels.size(); ++i) {
            OutputCompareChannel* oc = m_oc_channels[i];
            if (addr == oc->config.reg_oc) {
                oc->reg = (m_temp << 8) | data.value;

                if (m_mode.ocr == CFG::OCR_Unbuffered) {
                    m_counter.set_comp_value(i, oc->reg);

                    if (i == 0 && m_mode.top == CFG::Top_OnCompA)
                        update_top();
                }

                do_reschedule = true;
                break;
            }
            else if (m_config.is_16bits && addr == oc->config.reg_oc + 1) {
                m_temp = data.value;
                break;
            }
            else if (addr == oc->config.rb_force.addr && oc->config.rb_force.extract(data.value)) {
                if (!m_mode.disable_foc)
                    change_OC_state(i, AVR_TimerCounter::Event_Compare);
                clear_ioreg(oc->config.rb_force);
                break;
            }
            else if (addr == oc->config.rb_mode.addr) {
                do_com_reconfig = true;
                break;
            }
        }

        //Check if writing to the clock source bitfield
        if (addr == m_config.rb_clock.addr) {
            uint8_t reg_val = m_config.rb_clock.extract(data.value);
            auto cfg = find_reg_config_p<CFG::clock_config_t>(m_config.clocks, reg_val);
            if (cfg) {
                m_counter.set_tick_source(cfg->source);
                logger().dbg("Clock changed to 0x%02x", cfg->source);
                if (cfg->source == AVR_TimerCounter::Tick_Timer)
                    m_timer.set_prescaler(m_clk_ps_max, cfg->div);
            } else {
                logger().dbg("Unsupported clock setting: 0x%02x", reg_val);
                m_counter.set_tick_source(AVR_TimerCounter::Tick_Stopped);
            }
            do_reschedule = true;
        }

        if (addr == m_config.rbc_mode.addr_match(addr)) {
            uint8_t reg_val = read_ioreg(m_config.rbc_mode);
            auto mode_cfg = find_reg_config_p<CFG::mode_config_t>(m_config.modes, reg_val);
            if (mode_cfg) {
                m_mode = *mode_cfg;
                m_counter.set_slope_mode(m_mode.double_slope ?
                                         AVR_TimerCounter::Slope_Double :
                                         AVR_TimerCounter::Slope_Up);
                update_top();
            } else {
                logger().dbg("Unsupported mode setting: 0x%02x", reg_val);
            }
            do_com_reconfig = true;
        }

        //If we're writing a 1 to a interrupt flag bit, it cancels the corresponding interrupt if it
        //has not been executed yet
        if (addr == m_config.reg_int_flag) {
            //Check the flag for OVF
            if (m_config.vect_ovf.num && BITSET(data.value, m_config.vect_ovf.bit))
                m_intflag_ovf.clear_flag();

            //Check the flag for ICR
            if (m_config.vect_icr.num && BITSET(data.value, m_config.vect_icr.bit))
                m_intflag_icr.clear_flag();

            //Check the flag for each OC
            for (uint32_t i = 0; i < m_oc_channels.size(); ++i) {
                OutputCompareChannel* oc = m_oc_channels[i];
                if (oc->config.vector.num && BITSET(data.value, oc->config.vector.bit))
                    oc->intflag.clear_flag();
            }
        }
    }

    if (do_com_reconfig) {
        logger().dbg("Reconfiguring OC modes");
        for (uint32_t i = 0; i < m_oc_channels.size(); ++i) {
            OutputCompareChannel* oc = m_oc_channels[i];
            oc->mode = get_COM_config(read_ioreg(oc->config.rb_mode));
            bool oc_active = output_active(oc->mode, i);
            m_counter.set_comp_enabled(i, oc_active);
            //If the ocm is inactive, ensure the output level is reset
            if (!oc_active)
                m_signal.raise_u(Signal_CompOutput, 0, i);
        }
        do_reschedule = true;
    }

    if (do_reschedule)
        m_counter.reschedule();
}


void AVR_ArchAVR_Timer::update_top()
{
    long top;
    switch(m_mode.top) {

        case CFG::Top_OnFixed:
            top = (1 << (m_mode.fixed_top_exp + 8)) - 1; break;

        case CFG::Top_OnCompA:
            top = m_oc_channels.size() ? m_counter.comp_value(0) : (m_counter.wrap() - 1); break;

        case CFG::Top_OnIC:
            top = m_icr; break;

        case CFG::Top_OnMax:
        default:
            top = m_counter.wrap() - 1;
    }
    m_counter.set_top(top);
}


CFG::COM_config_t AVR_ArchAVR_Timer::get_COM_config(uint8_t regval)
{
    const std::vector<CFG::COM_config_t>& com_config = m_config.com_modes[m_mode.com_variant];
    auto com = find_reg_config_p<CFG::COM_config_t>(com_config, regval);
    if (com)
        return *com;
    else
        return { 0 };
}


/*
 * Determine is a COM mode is active, i.e. driving the OC output
 * output_index is the OC index (0=A, 1=B, ...)
 */
static inline bool COM_active(CFG::COM com, uint32_t output_index)
{
    if (output_index == 0)
        return (com > CFG::COM_NoChange);
    else
        return (com > CFG::COM_NoChange) && (com <= CFG::COM_Set);
}


/*
 * Determine if an OC is active. It is if any of its COM modes is active.
 */
bool AVR_ArchAVR_Timer::output_active(CFG::COM_config_t& mode, uint32_t output_index)
{
    return COM_active(mode.up, output_index) ||
           COM_active(mode.down, output_index) ||
           COM_active(mode.bottom, output_index) ||
           COM_active(mode.top, output_index);
}


void AVR_ArchAVR_Timer::raised(const signal_data_t& sigdata, uint16_t __unused)
{
    if (sigdata.sigid == AVR_TimerCounter::Signal_Event) {
        uint8_t event_flags = sigdata.data.as_uint();
        bool raise_ovf = false;

        //If the MAX value has been reached
        if (event_flags & AVR_TimerCounter::Event_Max) {
            if (m_mode.ovf == CFG::OVF_SetOnMax)
                raise_ovf = true;
        }

        //If the TOP value has been reached
        if (event_flags & AVR_TimerCounter::Event_Top) {
            if (m_mode.ocr == CFG::OCR_UpdateOnTop) {
                for (uint32_t i = 0; i < m_oc_channels.size(); ++i)
                    m_counter.set_comp_value(i, m_oc_channels[i]->reg);

                if (m_mode.top == CFG::Top_OnCompA)
                    update_top();
            }

            if (m_mode.ovf == CFG::OVF_SetOnTop && !m_counter.countdown())
                raise_ovf = true;

            //Process the OC channels, only if OCRx != TOP, otherwise
            //it would be called twice as the signal Signal_CompMatch
            //would also be raised in the same cycle
            if (!(event_flags & AVR_TimerCounter::Event_Compare)) {
                for (uint32_t i = 0; i < m_oc_channels.size(); ++i)
                    change_OC_state(i, event_flags);
            }

        }

        //If the BOTTOM value has been reached
        if (event_flags & AVR_TimerCounter::Event_Bottom) {
            if (m_mode.ocr == CFG::OCR_UpdateOnBottom) {
                for (uint32_t i = 0; i < m_oc_channels.size(); ++i)
                    m_counter.set_comp_value(i, m_oc_channels[i]->reg);

                if (m_mode.top == CFG::Top_OnCompA)
                    update_top();
            }

            if (m_mode.ovf == CFG::OVF_SetOnBottom && m_counter.countdown())
                raise_ovf = true;

            //Ditto as Event_Top
            if (!(event_flags & AVR_TimerCounter::Event_Compare)) {
                for (uint32_t i = 0; i < m_oc_channels.size(); ++i)
                    change_OC_state(i, event_flags);
            }
        }

        //If the condition to raise the Overflow Flag has been reached
        if (raise_ovf) {
            logger().dbg("Counter triggering OVF interrupt");
            m_intflag_ovf.set_flag();
            m_signal.raise_u(Signal_OVF, 0);
        }
    }

    else if (sigdata.sigid == AVR_TimerCounter::Signal_CompMatch) {
        logger().dbg("Triggering Compare Match %ud" , sigdata.index);
        change_OC_state(sigdata.index, sigdata.data.as_uint());
        //Raise the corresponding interrupt flag
        m_oc_channels[sigdata.index]->intflag.set_flag();
    }
}


void AVR_ArchAVR_Timer::change_OC_state(uint32_t index, uint8_t event_flags)
{
    OutputCompareChannel* oc = m_oc_channels[index];

    //Determine the action to perform on the Output Compare state depending
    //on the COM settings
    //TODO: Take into account edge cases (such as OCRx == BOTTOM or TOP)
    bool do_clear = false;
    bool do_set = false;
    bool do_toggle = false;

    if (event_flags & AVR_TimerCounter::Event_Compare) {
        if (m_counter.countdown()) {
            do_clear |= oc->mode.down == CFG::COM_Clear;
            do_set |= oc->mode.down == CFG::COM_Set;
            do_toggle |= oc->mode.down == CFG::COM_Toggle;
        } else {
            do_clear |= oc->mode.up == CFG::COM_Clear;
            do_set |= oc->mode.up == CFG::COM_Set;
            do_toggle |= oc->mode.up == CFG::COM_Toggle;
        }
    }

    if (event_flags & AVR_TimerCounter::Event_Top) {
        do_clear |= oc->mode.top == CFG::COM_Clear;
        do_set |= oc->mode.top == CFG::COM_Set;
        do_toggle |= oc->mode.top == CFG::COM_Toggle;
    }

    if (event_flags & AVR_TimerCounter::Event_Bottom) {
        do_clear |= oc->mode.bottom == CFG::COM_Clear;
        do_set |= oc->mode.bottom == CFG::COM_Set;
        do_toggle |= oc->mode.bottom == CFG::COM_Toggle;
    }

    unsigned char old_state = oc->state;
    if (do_clear && !do_set)
        oc->state = 0;
    else if (do_set && !do_clear)
        oc->state = 1;
    else if (do_toggle && !(do_set || do_clear))
        oc->state ^= 1;

    if (oc->state != old_state)
        m_signal.raise_u(Signal_CompOutput, oc->state ? 1 : 0, index);
}


void AVR_ArchAVR_Timer::capt_raised()
{
    //If no ICR is registered, the Input Capture function is unavailable
    if (!m_config.reg_icr.valid()) return;

    //If ICR is used for TOP value, the Input Capture function is disabled
    if (m_mode.top == CFG::Top_OnIC) return;

    m_timer.update();
    m_icr = m_counter.counter();
    m_intflag_icr.set_flag();
    m_signal.raise_u(Signal_Capt, 0);
}
