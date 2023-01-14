/*
 * arch_mega0_timer_b.cpp
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

#include "arch_mega0_timer_b.h"
#include "arch_mega0_timer_a.h"
#include "arch_mega0_io.h"
#include "arch_mega0_io_utils.h"
#include "core/sim_device.h"
#include "core/sim_sleep.h"


//=======================================================================================

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(TCB_t, reg))

#define REG_OFS(reg) \
    offsetof(TCB_t, reg)

#define TIMER_CLOCK_DISABLED 0xFF

static const char* ClockSourceNames[3] = {
    "ClkDiv1",
    "ClkDiv2",
    "TCA"
};


AVR_ArchMega0_TimerB::AVR_ArchMega0_TimerB(int num, const AVR_ArchMega0_TimerB_Config& config)
:AVR_Peripheral(AVR_IOCTL_TIMER('B', 0x30 + num))
,m_config(config)
,m_clk_mode(TIMER_CLOCK_DISABLED)
,m_cnt(0)
,m_ccmp(0)
,m_intflag(false)
,m_next_event_type(0)
{}

bool AVR_ArchMega0_TimerB::init(AVR_Device& device)
{
    bool status = AVR_Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), TCB_CLKSEL_gm | TCB_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), TCB_CNTMODE_gm);
    //EVCTRL not implemented
    add_ioreg(REG_ADDR(INTCTRL), TCB_CAPT_bm);
    add_ioreg(REG_ADDR(INTFLAGS), TCB_CAPT_bm);
    //STATUS not implemented
    //DBGCTRL not supported
    add_ioreg(REG_ADDR(TEMP));
    add_ioreg(REG_ADDR(CNTL));
    add_ioreg(REG_ADDR(CNTH));
    add_ioreg(REG_ADDR(CCMPL));
    add_ioreg(REG_ADDR(CCMPH));

    status &= m_intflag.init(device,
                             DEF_REGBIT_B(INTCTRL, TCB_CAPT),
                             DEF_REGBIT_B(INTFLAGS, TCB_CAPT),
                             m_config.iv_capt);

    m_timer.init(*device.cycle_manager(), logger());
    m_timer.signal().connect_hook(this);

    return status;
}

void AVR_ArchMega0_TimerB::reset()
{
    AVR_Peripheral::reset();
    m_clk_mode = TIMER_CLOCK_DISABLED;
    m_cnt = 0;
    m_ccmp = 0;
    m_next_event_type = 0;
    m_timer.reset();
}

uint8_t AVR_ArchMega0_TimerB::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //16-bits reading of CNT
    if (reg_ofs == REG_OFS(CNTL)) {
        m_timer.update();
        value = m_cnt & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), m_cnt >> 8);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    //16-bits reading of CCMP
    else if (reg_ofs == REG_OFS(CCMPL)) {
        value = m_ccmp & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), m_ccmp >> 8);
    }
    else if (reg_ofs == REG_OFS(CCMPH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    return value;
}

void AVR_ArchMega0_TimerB::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    bool do_reschedule = false;
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        uint8_t old_clk_mode = m_clk_mode;

        if (data.value & TCB_ENABLE_bm) {
            //Extract the clock mode from the bit fields
            bitmask_t bm_clksel = DEF_BITMASK_F(TCB_CLKSEL);
            m_clk_mode = data.value & TCB_CLKSEL_gm;
            //Pretty print on the logger
            const char* clk_name = ClockSourceNames[bm_clksel.extract(data.value)];
            logger().dbg("Clock changed to SRC=%s", clk_name);
        } else {
            m_clk_mode = TIMER_CLOCK_DISABLED;
            logger().dbg("Disabled");
        }

        //Check if we need to chain or de-chain the TCB timer from TCA
        bool is_chained = (old_clk_mode == TCB_CLKSEL_CLKTCA_gc);
        bool to_chain = (m_clk_mode == TCB_CLKSEL_CLKTCA_gc);
        if (to_chain != is_chained) {
            ctlreq_data_t d = { .data = &m_timer, .index = (uint32_t)(to_chain ? 1 : 0) };
            device()->ctlreq(AVR_IOCTL_TIMER('A', '0'), AVR_CTLREQ_TCA_REGISTER_TCB, &d);
        }

        //If the TCB clock mode has changed, reconfigure the timer prescaler
        if (m_clk_mode != old_clk_mode) {
            if (m_clk_mode == TCB_CLKSEL_CLKDIV1_gc || m_clk_mode == TCB_CLKSEL_CLKTCA_gc)
                m_timer.set_prescaler(2, 1);
            else if (m_clk_mode == TCB_CLKSEL_CLKDIV2_gc)
                m_timer.set_prescaler(2, 2);
            else //disabled
                m_timer.set_prescaler(2, 0);
        }

        do_reschedule = true;
    }

    //16-bits writing to CNT
    else if (reg_ofs == REG_OFS(CNTL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_cnt = temp | (data.value << 8);
        do_reschedule = true;
    }

    //16-bits writing to CCMP
    else if (reg_ofs == REG_OFS(CCMPL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(CCMPH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_ccmp = temp | (data.value << 8);
        do_reschedule = true;
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_intflag.update_from_ioreg();
    }

    //If we're writing a 1 to the interrupt flag bit, it clears the bit and cancels the interrupt
    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        if (data.value & TCB_CAPT_bm)
            m_intflag.clear_flag();
    }

    if (do_reschedule)
        m_timer.set_timer_delay(delay_to_event());
}

/*
 * Defines the types of 'event' that can be triggered by the counter
 */
enum TimerEventType {
    TimerEventMax       = 0x01,
    TimerEventComp      = 0x02,
};

/*
 * Calculates the delay in prescaler ticks and the type of the next timer/counter event
 */
uint32_t AVR_ArchMega0_TimerB::delay_to_event()
{
    int ticks_to_max = AVR_PrescaledTimer::ticks_to_event(m_cnt, 0xFFFF, 0x10000);
    int ticks_to_next_event = ticks_to_max;

    int ticks_to_comp = AVR_PrescaledTimer::ticks_to_event(m_cnt, m_ccmp, 0x10000);
    if (ticks_to_comp < ticks_to_next_event)
        ticks_to_next_event = ticks_to_comp;

    m_next_event_type = 0;
    if (ticks_to_next_event == ticks_to_max)
        m_next_event_type |= TimerEventMax;
    if (ticks_to_next_event == ticks_to_comp)
        m_next_event_type |= TimerEventComp;

    logger().dbg("Next event: 0x%x in %d cycles", m_next_event_type, ticks_to_next_event);

    return ticks_to_next_event;
}

void AVR_ArchMega0_TimerB::raised(const signal_data_t& sigdata, uint16_t __unused)
{
    m_cnt += sigdata.data.as_uint();

    if (!sigdata.index) return;

    //From this point onwards, we know we have an 'event' to process.

    if (m_next_event_type & TimerEventComp) {
        m_intflag.set_flag();
        m_cnt = 0;
    }

    if (m_next_event_type & TimerEventMax)
        m_cnt = 0;

    //Reconfigure the timer delay to the next event
    m_timer.set_timer_delay(delay_to_event());
}

void AVR_ArchMega0_TimerB::sleep(bool on, AVR_SleepMode mode)
{
    //The timer is paused for sleep modes above Standby and in Standby if RUNSTDBY bit is not set
    bool stbyrun_set = TEST_IOREG(CTRLA, TCB_RUNSTDBY);
    if (mode > AVR_SleepMode::Standby || (mode == AVR_SleepMode::Standby && !stbyrun_set)) {
        m_timer.set_paused(on);
        logger().dbg(on ? "Pausing" : "Resuming");
    }
}
