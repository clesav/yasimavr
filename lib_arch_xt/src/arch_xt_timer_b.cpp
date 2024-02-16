/*
 * arch_xt_timer_b.cpp
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

#include "arch_xt_timer_b.h"
#include "arch_xt_timer_a.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"
#include "core/sim_sleep.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(TCB_t, reg))

#define REG_OFS(reg) \
    offsetof(TCB_t, reg)

#define TIMER_CLOCK_DISABLED -1


enum OutputChange {
    Output_NoChange = 0,
    Output_Clear,
    Output_Set,
    Output_Reset,
};


typedef ArchXT_TimerBConfig CFG;


class ArchXT_TimerB::EventHook : public SignalHook {

public:

    explicit EventHook(ArchXT_TimerB& tmr) : m_tmr(tmr) {}

    virtual void raised(const signal_data_t& sigdata, int hooktag) override
    {
        m_tmr.hook_raised(sigdata, hooktag);
    }

private:

    ArchXT_TimerB& m_tmr;

};


ArchXT_TimerB::ArchXT_TimerB(int num, const CFG& config)
:Peripheral(AVR_IOCTL_TIMER('B', 0x30 + num))
,m_config(config)
,m_clk_mode(TIMER_CLOCK_DISABLED)
,m_cnt_mode(TCB_CNTMODE_INT_gc)
,m_cnt_state(State_Run)
,m_ccmp(0)
,m_event_state(0)
,m_output(0)
,m_intflag(false)
,m_counter(0x10000, 1)
{
    m_event_hook = new EventHook(*this);
}


ArchXT_TimerB::~ArchXT_TimerB()
{
    delete m_event_hook;
}


bool ArchXT_TimerB::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), TCB_RUNSTDBY_bm | TCB_CASCADE_bm | TCB_CLKSEL_gm | TCB_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), TCB_ASYNC_bm | TCB_CCMPINIT_bm | TCB_CCMPEN_bm | TCB_CNTMODE_gm);
    add_ioreg(REG_ADDR(EVCTRL), TCB_FILTER_bm | TCB_EDGE_bm | TCB_CAPTEI_bm);
    add_ioreg(REG_ADDR(STATUS), TCB_RUN_bm, true);
    //DBGCTRL not supported
    add_ioreg(REG_ADDR(TEMP));
    add_ioreg(REG_ADDR(CNTL));
    add_ioreg(REG_ADDR(CNTH));
    add_ioreg(REG_ADDR(CCMPL));
    add_ioreg(REG_ADDR(CCMPH));

    uint8_t iv_flags = TCB_CAPT_bm;
    if (m_config.options & CFG::OverflowFlag)
        iv_flags |= TCB_OVF_bm;

    add_ioreg(REG_ADDR(INTCTRL), iv_flags);
    add_ioreg(REG_ADDR(INTFLAGS), iv_flags);

    status &= m_intflag.init(device,
                             regbit_t(REG_ADDR(INTCTRL), 0, iv_flags),
                             regbit_t(REG_ADDR(INTFLAGS), 0, iv_flags),
                             m_config.iv_capt);

    m_counter.init(*device.cycle_manager(), logger());
    m_counter.signal().connect(*this);

    return status;
}


void ArchXT_TimerB::reset()
{
    Peripheral::reset();
    m_clk_mode = TIMER_CLOCK_DISABLED;
    m_cnt_mode = TCB_CNTMODE_INT_gc;
    set_counter_state(State_Run);
    m_ccmp = 0;
    m_counter.reset();
    m_counter.set_top(0);
    m_counter.set_comp_value(0, 0);
    m_event_state = 0;
    m_intflag.update_from_ioreg();
    update_output(Output_Reset);
}


bool ArchXT_TimerB::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_TCB_GET_EVENT_HOOK) {
        data->data = m_event_hook;
        return true;
    }
    return false;
}


uint8_t ArchXT_TimerB::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //16-bits reading of CNT
    if (reg_ofs == REG_OFS(CNTL)) {
        m_counter.update();
        uint16_t v = m_counter.counter();
        value = v & 0x00FF;
        WRITE_IOREG(TEMP, v >> 8);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        value = READ_IOREG(TEMP);
    }

    //16-bits reading of CCMP
    else if (reg_ofs == REG_OFS(CCMPL)) {
        value = m_ccmp & 0x00FF;
        WRITE_IOREG(TEMP, m_ccmp >> 8);
        update_on_CCMP_read();
    }
    else if (reg_ofs == REG_OFS(CCMPH)) {
        value = READ_IOREG(TEMP);
    }

    return value;
}


void ArchXT_TimerB::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        m_counter.update();
        uint8_t old_clk_mode = m_clk_mode;

        if (data.value & TCB_ENABLE_bm)
            m_clk_mode = data.value & TCB_CLKSEL_gm;
        else
            m_clk_mode = TIMER_CLOCK_DISABLED;

        //Check if we need to chain or de-chain the TCB timer from TCA
        bool is_chained = (old_clk_mode == TCB_CLKSEL_TCA0_gc);
        bool to_chain = (m_clk_mode == TCB_CLKSEL_TCA0_gc);
        if (to_chain != is_chained) {
            ctlreq_data_t d = { .data = &m_counter.prescaler(),
                                .index = to_chain ? 1 : 0 };
            device()->ctlreq(AVR_IOCTL_TIMER('A', '0'), AVR_CTLREQ_TCA_REGISTER_TCB, &d);
        }

        //If the TCB clock mode has changed, reconfigure the timer prescaler
        if (m_clk_mode != old_clk_mode) {
            if (m_clk_mode == TCB_CLKSEL_DIV1_gc || m_clk_mode == TCB_CLKSEL_TCA0_gc)
                m_counter.prescaler().set_prescaler(2, 1);
            else if (m_clk_mode == TCB_CLKSEL_DIV2_gc)
                m_counter.prescaler().set_prescaler(2, 2);
            else //disabled
                m_counter.prescaler().set_prescaler(2, 0);
        }

        set_counter_state(m_cnt_state);
        m_counter.reschedule();
        update_output(Output_NoChange);
    }

    else if (reg_ofs == REG_OFS(CTRLB)) {
        m_counter.update();
        m_cnt_mode = data.value & TCB_CNTMODE_gm;
        if (m_cnt_mode == TCB_CNTMODE_TIMEOUT_gc || m_cnt_mode == TCB_CNTMODE_FRQPW_gc ||
            m_cnt_mode == TCB_CNTMODE_SINGLE_gc)
            set_counter_state(State_Ready);
        else
            set_counter_state(State_Run);

        m_counter.set_comp_enabled(0, m_cnt_mode == TCB_CNTMODE_PWM8_gc ||
                                      m_cnt_mode == TCB_CNTMODE_TIMEOUT_gc);
        update_counter_top();
        m_counter.reschedule();
        update_output(Output_Reset);
    }

    //16-bits writing to CNT
    else if (reg_ofs == REG_OFS(CNTL)) {
        WRITE_IOREG(TEMP, data.value);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        m_counter.update();
        m_counter.set_counter(READ_IOREG(TEMP) | (data.value << 8));
        m_counter.reschedule();
    }

    //16-bits writing to CCMP
    else if (reg_ofs == REG_OFS(CCMPL)) {
        WRITE_IOREG(TEMP, data.value);
    }
    else if (reg_ofs == REG_OFS(CCMPH)) {
        m_ccmp = READ_IOREG(TEMP) | (data.value << 8);
        m_counter.update();
        m_counter.set_comp_value(0, m_ccmp);
        update_counter_top();
        m_counter.reschedule();
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_intflag.update_from_ioreg();
    }

    //If we're writing a 1 to the interrupt flag bit, it clears the bit and cancels the interrupt
    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        if (data.value & TCB_CAPT_bm)
            m_intflag.clear_flag(TCB_CAPT_bm);
        if ((data.value & TCB_OVF_bm) && (m_config.options & CFG::OverflowFlag))
            m_intflag.clear_flag(TCB_OVF_bm);
    }
}


void ArchXT_TimerB::set_counter_state(State state)
{
    m_cnt_state = state;

    TimerCounter::TickSource src;
    if (m_clk_mode == TIMER_CLOCK_DISABLED || state != State_Run)
        src = TimerCounter::Tick_Stopped;
    else if (m_clk_mode == TCB_CLKSEL_EVENT_gc)
        src = TimerCounter::Tick_External;
    else
        src = TimerCounter::Tick_Timer;

    m_counter.set_tick_source(src);

    WRITE_IOREG_B(STATUS, TCB_RUN, src == TimerCounter::Tick_Stopped ? 0 : 1);
}


void ArchXT_TimerB::update_counter_top()
{
    long top;

    if (m_cnt_mode == TCB_CNTMODE_INT_gc || m_cnt_mode == TCB_CNTMODE_SINGLE_gc)
        top = m_ccmp;
    else if (m_cnt_mode == TCB_CNTMODE_PWM8_gc)
        top = m_ccmp & 0x00FF;
    else
        top = 0xFFFF;

    m_counter.set_top(top);
}


void ArchXT_TimerB::update_on_CCMP_read()
{
    if (m_cnt_mode == TCB_CNTMODE_CAPT_gc || m_cnt_mode == TCB_CNTMODE_FRQ_gc ||
        m_cnt_mode == TCB_CNTMODE_PW_gc || m_cnt_mode == TCB_CNTMODE_FRQPW_gc)

        m_intflag.clear_flag(TCB_CAPT_bm);

    if (m_cnt_mode == TCB_CNTMODE_FRQPW_gc && m_cnt_state == State_End)
        m_cnt_state = State_Ready;
}


void ArchXT_TimerB::raised(const signal_data_t& data, int hooktag)
{
    if (data.sigid != TimerCounter::Signal_Event) return;

    int event_flags = data.data.as_uint();

    if (event_flags & TimerCounter::Event_Top) {
        if (m_cnt_mode == TCB_CNTMODE_INT_gc || m_cnt_mode == TCB_CNTMODE_PWM8_gc) {
            raise_capture_flag();
        }
        else if (m_cnt_mode == TCB_CNTMODE_SINGLE_gc) {
            raise_capture_flag();
            update_output(Output_Clear);
            set_counter_state(State_Ready);
        }
    }

    if (event_flags & TimerCounter::Event_Bottom) {
        if (m_cnt_mode == TCB_CNTMODE_PWM8_gc)
            update_output(Output_Set);
    }

    if (event_flags & TimerCounter::Event_Compare) {
        if (m_cnt_mode == TCB_CNTMODE_TIMEOUT_gc)
            raise_capture_flag();
        else if (m_cnt_mode == TCB_CNTMODE_PWM8_gc)
            update_output(Output_Clear);
    }

    if (event_flags & TimerCounter::Event_Max) {
        if (m_config.options & CFG::OverflowFlag)
            m_intflag.set_flag(TCB_OVF_bm);
    }
}


void ArchXT_TimerB::hook_raised(const signal_data_t& sigdata, int hooktag)
{
    if (hooktag == Tag_Event) {
        unsigned char event_state = sigdata.data.as_uint() ? 1 : 0;
        process_capture_event(event_state);
    }
    else if (hooktag == Tag_Count && (m_config.options & CFG::EventCount) && m_clk_mode == TCB_CLKSEL_EVENT_gc) {
        m_counter.tick();
    }
}


void ArchXT_TimerB::process_capture_event(unsigned char event_state)
{
    //Return if we don't have an edge
    if (event_state == m_event_state) return;
    m_event_state = event_state;

    //Return if the timer and the capture are not both enabled
    if (!TEST_IOREG(CTRLA, TCB_ENABLE)) return;
    if (!TEST_IOREG(EVCTRL, TCB_CAPTEI)) return;

    logger().dbg("Captured edge %s", event_state ? "rising" : "falling");

    //Process the effect of the bit EDGE in EVCTRL
    bool edge_value;
    if (m_cnt_mode == TCB_CNTMODE_SINGLE_gc)
        //Special case of SINGLE mode : any edge triggers the counter if EDGE is set
        edge_value = TEST_IOREG(EVCTRL, TCB_EDGE) || event_state;
    else
        //All other modes, invert the edge value if EDGE is set
        edge_value = TEST_IOREG(EVCTRL, TCB_EDGE) ? (!event_state) : (!!event_state);

    m_counter.update();

    //Hopefully this switch is self-explanatory when read alongside the MCU datasheet
    switch(m_cnt_mode) {
        case TCB_CNTMODE_TIMEOUT_gc:
            if (edge_value) {
                m_counter.set_counter(0);
                set_counter_state(State_Run);
            } else {
                set_counter_state(State_Ready);
            }
            break;

        case TCB_CNTMODE_CAPT_gc:
            if (edge_value) {
                m_ccmp = m_counter.counter();
                raise_capture_flag();
            }
            break;

        case TCB_CNTMODE_FRQ_gc:
            if (edge_value) {
                m_ccmp = m_counter.counter();
                m_counter.set_counter(0);
                raise_capture_flag();
            }
            break;

        case TCB_CNTMODE_PW_gc:
            if (edge_value) {
                m_counter.set_counter(0);
            } else {
                m_ccmp = m_counter.counter();
                raise_capture_flag();
            }
            break;

        case TCB_CNTMODE_FRQPW_gc:
            if (edge_value) {
                if (m_cnt_state == State_Ready) {
                    set_counter_state(State_Run);
                    m_counter.set_counter(0);
                }
                else if (m_cnt_state == State_Run) {
                    set_counter_state(State_End);
                    raise_capture_flag();
                }
            }
            else if (m_cnt_state == State_Run) {
                m_ccmp = m_counter.counter();
            }
            break;

        case TCB_CNTMODE_SINGLE_gc:
            if (edge_value && m_cnt_state == State_Ready) {
                set_counter_state(State_Run);
                m_counter.set_counter(0);
                update_output(Output_Set);
            }
            break;

        default:
            logger().dbg("Captured edge ignored");
            break;
    }

    m_counter.reschedule();
}


void ArchXT_TimerB::raise_capture_flag()
{
    m_intflag.set_flag(TCB_CAPT_bm);
    m_signal.raise(Signal_Capture);
    logger().dbg("Capture flag raised");
}


/*
 * Update the Compare Output value.
 * If the output is enabled (bit CCMPEN in CTRLB register), raise the signal
 * or else, update the value silently.
 */
void ArchXT_TimerB::update_output(int change)
{
    switch(change) {
        case Output_Set:
            m_output = 1; break;

        case Output_Clear:
            m_output = 0; break;

        case Output_Reset:
            if (m_cnt_mode == TCB_CNTMODE_SINGLE_gc || m_cnt_mode == TCB_CNTMODE_PWM8_gc)
                m_output = 0;
            else
                m_output = READ_IOREG_B(CTRLB, TCB_CCMPINIT);
            break;

        default: break;
    }

    vardata_t sigdata;
    if (TEST_IOREG(CTRLA, TCB_ENABLE) && TEST_IOREG(CTRLB, TCB_CCMPEN))
        sigdata = m_output;

    if (sigdata != m_signal.data(Signal_Output))
        m_signal.raise(Signal_Output, sigdata);
}


void ArchXT_TimerB::sleep(bool on, SleepMode mode)
{
    //The timer is paused for sleep modes above Standby and in Standby if RUNSTDBY bit is not set
    bool stbyrun_set = TEST_IOREG(CTRLA, TCB_RUNSTDBY);
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !stbyrun_set)) {
        m_counter.prescaler().set_paused(on);
        logger().dbg(on ? "Pausing" : "Resuming");
    }
}
