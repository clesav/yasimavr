/*
 * arch_mega0_rtc.cpp
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

#include "arch_mega0_rtc.h"
#include "arch_mega0_io.h"
#include "arch_mega0_io_utils.h"
#include "core/sim_device.h"
#include "core/sim_sleep.h"


//=======================================================================================
//Macros definition to access the registers of the controller

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(RTC_t, reg))

#define REG_OFS(reg) \
    offsetof(RTC_t, reg)

typedef AVR_ArchMega0_RTC_Config CFG;

#define PRESCALER_MAX 32678

//Enum values for the m_clk_mode variables;
enum RTC_Mode {
    RTC_Disabled = 0x00,
    RTC_Enabled = 0x01,
    PIT_Enabled = 0x02
};


//=======================================================================================

class AVR_ArchMega0_RTC::TimerHook : public AVR_SignalHook {

public:

    explicit TimerHook(AVR_ArchMega0_RTC& ctl) : m_ctl(ctl) {}

    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override {
        if (hooktag)
            m_ctl.pit_hook_raised(sigdata);
        else
            m_ctl.rtc_hook_raised(sigdata);
    }

private:

    AVR_ArchMega0_RTC& m_ctl;

};


AVR_ArchMega0_RTC::AVR_ArchMega0_RTC(const CFG& config)
:AVR_Peripheral(AVR_IOCTL_RTC)
,m_config(config)
,m_clk_mode(RTC_Disabled)
,m_rtc_cnt(0)
,m_rtc_per(0)
,m_rtc_cmp(0)
,m_pit_cnt(0)
,m_next_rtc_event_type(0)
,m_rtc_intflag(false)
,m_pit_intflag(false)
{
    m_timer_hook = new TimerHook(*this);
    m_rtc_timer.signal().connect_hook(m_timer_hook, 0);
    m_pit_timer.signal().connect_hook(m_timer_hook, 1);
}

AVR_ArchMega0_RTC::~AVR_ArchMega0_RTC()
{
    delete m_timer_hook;
}

bool AVR_ArchMega0_RTC::init(AVR_Device& device)
{
    bool status = AVR_Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), RTC_RUNSTDBY_bm | RTC_PRESCALER_gm | RTC_RTCEN_bm);
    add_ioreg(REG_ADDR(STATUS), RTC_CTRLABUSY_bm, true);
    add_ioreg(REG_ADDR(INTCTRL), RTC_CMP_bm | RTC_OVF_bm);
    add_ioreg(REG_ADDR(INTFLAGS), RTC_CMP_bm | RTC_OVF_bm);
    add_ioreg(REG_ADDR(TEMP));
    //DBGCTRL not supported
    //CALIB not supported
    add_ioreg(REG_ADDR(CLKSEL), RTC_CLKSEL_gm);
    add_ioreg(REG_ADDR(CNTL));
    add_ioreg(REG_ADDR(CNTH));
    add_ioreg(REG_ADDR(PERL));
    add_ioreg(REG_ADDR(PERH));
    add_ioreg(REG_ADDR(CMPL));
    add_ioreg(REG_ADDR(CMPH));
    add_ioreg(REG_ADDR(PITCTRLA), RTC_PERIOD_gm | RTC_PITEN_bm);
    add_ioreg(REG_ADDR(PITSTATUS), RTC_CTRLBUSY_bm, true);
    add_ioreg(REG_ADDR(PITINTCTRL), RTC_PI_bm);
    add_ioreg(REG_ADDR(PITINTFLAGS), RTC_PI_bm);
    //PITDBGCTRL not supported

    status &= m_rtc_intflag.init(device,
                                 regbit_t(REG_ADDR(INTCTRL), 0, RTC_CMP_bm | RTC_OVF_bm),
                                 regbit_t(REG_ADDR(INTFLAGS), 0, RTC_CMP_bm | RTC_OVF_bm),
                                 m_config.iv_rtc);
    status &= m_pit_intflag.init(device,
                                 DEF_REGBIT_B(PITINTCTRL, RTC_PI),
                                 DEF_REGBIT_B(PITINTFLAGS, RTC_PI),
                                 m_config.iv_pit);

    m_rtc_timer.init(device.cycle_manager(), logger());
    m_pit_timer.init(device.cycle_manager(), logger());

    return status;
}

void AVR_ArchMega0_RTC::reset()
{
    m_clk_mode = RTC_Disabled;
    m_rtc_cnt = 0;
    m_rtc_per = 0xFFFF;
    write_ioreg(REG_ADDR(PERL), 0xFF);
    write_ioreg(REG_ADDR(PERH), 0xFF);
    m_rtc_cmp = 0;
    m_pit_cnt = 0;
    m_next_rtc_event_type = 0;
    m_rtc_timer.reset();
    m_pit_timer.reset();
}

void AVR_ArchMega0_RTC::ioreg_read_handler(reg_addr_t addr)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //16-bits reading of CNT
    if (reg_ofs == REG_OFS(CNTL)) {
        m_rtc_timer.update();
        write_ioreg(REG_ADDR(CNTL), m_rtc_cnt & 0x00FF);
        write_ioreg(REG_ADDR(TEMP), m_rtc_cnt >> 8);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        write_ioreg(REG_ADDR(CNTH), read_ioreg(REG_ADDR(TEMP)));
    }

    //16-bits reading of PER
    else if (reg_ofs == REG_OFS(PERL)) {
        write_ioreg(REG_ADDR(PERL), m_rtc_per & 0x00FF);
        write_ioreg(REG_ADDR(TEMP), m_rtc_per >> 8);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        write_ioreg(REG_ADDR(PERH), read_ioreg(REG_ADDR(TEMP)));
    }

    //16-bits reading of CMP
    else if (reg_ofs == REG_OFS(CMPL)) {
        write_ioreg(REG_ADDR(CMPL), m_rtc_cmp & 0x00FF);
        write_ioreg(REG_ADDR(TEMP), m_rtc_cmp >> 8);
    }
    else if (reg_ofs == REG_OFS(CMPH)) {
        write_ioreg(REG_ADDR(CMPH), read_ioreg(REG_ADDR(TEMP)));
    }
}

void AVR_ArchMega0_RTC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    const reg_addr_t reg_ofs = addr - m_config.reg_base;
    bool do_reconfigure = false;
    bool do_rtc_reschedule = false;

    if (reg_ofs == REG_OFS(CTRLA)) {
        if (TEST_IOREG(CTRLA, RTC_RTCEN))
            m_clk_mode |= RTC_Enabled;
        else
            m_clk_mode &= ~RTC_Enabled;

        do_reconfigure = (data.value != data.old);
    }

    else if (reg_ofs == REG_OFS(PITCTRLA)) {
        if (TEST_IOREG(PITCTRLA, RTC_PITEN))
            m_clk_mode |= PIT_Enabled;
        else
            m_clk_mode &= ~PIT_Enabled;

        do_reconfigure = (data.value != data.old);
    }

    else if (reg_ofs== REG_OFS(CLKSEL)) {
        do_reconfigure = (data.value != data.old);
    }

    //16-bits writing to CNT
    else if (reg_ofs == REG_OFS(CNTL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_rtc_cnt = temp | (data.value << 8);
        do_rtc_reschedule = true;
    }

    //16-bits writing to PER
    else if (reg_ofs == REG_OFS(PERL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_rtc_timer.update();
        m_rtc_per = temp | (data.value << 8);
        do_rtc_reschedule = true;
    }

    //16-bits writing to CMP
    else if (reg_ofs == REG_OFS(CMPL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(CMPH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_rtc_timer.update();
        m_rtc_cmp = temp | (data.value << 8);
        do_rtc_reschedule = true;
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_rtc_intflag.update_from_ioreg();
    }

    //If we're writing a 1 to one of the RTC interrupt flag bits,
    //it clears the corresponding bits.
    //If all bits are clear, cancel the interrupt
    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        bitmask_t bm = bitmask_t(0, RTC_CMP_bm | RTC_OVF_bm);
        write_ioreg(addr, bm.clear_from(data.value));
        m_rtc_intflag.clear_flag(bm.extract(data.value));
    }

    else if (reg_ofs == REG_OFS(PITINTCTRL)) {
        m_pit_intflag.update_from_ioreg();
    }

    //If we're writing a 1 to the PIT interrupt flag bit, it clears the bit
    //and cancel the interrupt
    else if (reg_ofs == REG_OFS(PITINTFLAGS)) {
        bitmask_t bm = bitmask_t(0, RTC_PI_bm);
        write_ioreg(addr, bm.clear_from(data.value));
        m_pit_intflag.clear_flag();
    }

    if (do_reconfigure) {
        configure_timers();
    }
    else if (do_rtc_reschedule) {
        m_rtc_timer.set_timer_delay((m_clk_mode | RTC_Enabled) ? rtc_delay() : 0);
    }
}

void AVR_ArchMega0_RTC::configure_timers()
{
    if (m_clk_mode) {
        //Read and configure the clock source
        uint8_t clk_mode_val = READ_IOREG_F(CLKSEL, RTC_CLKSEL);
        int clk_mode_ix = find_reg_config<CFG::clksel_config_t>(m_config.clocks, clk_mode_val);
        CFG::RTC_ClockSource clk_src = m_config.clocks[clk_mode_ix].source;
        uint32_t clk_factor; //ratio (main MCU clock freq) / (RTC clock freq)
        if (clk_src == CFG::Clock_32kHz)
            clk_factor = device()->frequency() / 32768;
        else if (clk_src == CFG::Clock_1kHz)
            clk_factor = device()->frequency() / 1024;
        else {
            device()->crash(CRASH_INVALID_CONFIG, "Invalid RTC clock source");
            return;
        }

        //Read and configure the prescaler factor
        const uint32_t ps_max = PRESCALER_MAX * clk_factor;
        const uint32_t f = (1 << READ_IOREG_F(CTRLA, RTC_PRESCALER)) * clk_factor;
        m_rtc_timer.set_prescaler(ps_max, f);
        m_pit_timer.set_prescaler(ps_max, f);

        //Set the delay
        m_rtc_timer.set_timer_delay((m_clk_mode | RTC_Enabled) ? rtc_delay() : 0);
        m_pit_timer.set_timer_delay((m_clk_mode | PIT_Enabled) ? pit_delay() : 0);

    } else {

        m_rtc_timer.set_prescaler(PRESCALER_MAX, 0);
        m_pit_timer.set_prescaler(PRESCALER_MAX, 0);

    }
}

/*
 * Defines the types of 'event' that can be triggered by the counter
 */
enum TimerEventType {
    TimerEventMax       = 0x01,
    TimerEventPer       = 0x02,
    TimerEventCmp       = 0x04,
};

/*
 * Calculates the delay in prescaler ticks and the type of the next timer/counter event
 * for the RTC part
 */

uint32_t AVR_ArchMega0_RTC::rtc_delay()
{
    int ticks_to_max = AVR_PrescaledTimer::ticks_to_event(m_rtc_cnt, 0x10000, 0x10000);
    int ticks_to_next_event = ticks_to_max;

    int ticks_to_per = AVR_PrescaledTimer::ticks_to_event(m_rtc_cnt, m_rtc_per, 0x10000);
    if (ticks_to_per < ticks_to_next_event)
        ticks_to_next_event = ticks_to_per;

    int ticks_to_cmp = AVR_PrescaledTimer::ticks_to_event(m_rtc_cnt, m_rtc_cmp, 0x10000);
    if (ticks_to_cmp < ticks_to_next_event)
        ticks_to_next_event = ticks_to_cmp;

    m_next_rtc_event_type = 0;
    if (ticks_to_next_event == ticks_to_max)
        m_next_rtc_event_type |= TimerEventMax;
    if (ticks_to_next_event == ticks_to_per)
        m_next_rtc_event_type |= TimerEventPer;
    if (ticks_to_next_event == ticks_to_cmp)
        m_next_rtc_event_type |= TimerEventCmp;

    logger().dbg("Next RTC event : 0x%x in %d cycles", m_next_rtc_event_type, ticks_to_next_event);

    return ticks_to_next_event;
}

/*
 * Calculates the delay in prescaler ticks for the PIT part
 */
uint32_t AVR_ArchMega0_RTC::pit_delay()
{
    uint8_t period_index = READ_IOREG_F(PITCTRLA, RTC_PERIOD);
    if (period_index == 0 || period_index == 0xFF) return 0;

    int period = 1 << (period_index + 1); //1 is DIV/4, 2 is DIV/8, etc

    int ticks_to_per = AVR_PrescaledTimer::ticks_to_event(m_pit_cnt, period - 1, period);

    logger().dbg("Next PIT event in %d cycles", ticks_to_per);

    return ticks_to_per;
}

/*
 *  Callback when entering sleep mode
 */
void AVR_ArchMega0_RTC::sleep(bool on, AVR_SleepMode mode)
{
    //The RTC timer is paused for sleep modes above Standby and in Standby if RTC_RUNSTDBY is not set
    //The PIT timer is not affected by sleep modes
    if (mode > AVR_SleepMode::Standby || (mode == AVR_SleepMode::Standby && !TEST_IOREG(CTRLA, RTC_RUNSTDBY)))
        m_rtc_timer.set_paused(on);
}

/*
 *  Callbacks for the prescaled timers
 */
void AVR_ArchMega0_RTC::rtc_hook_raised(const signal_data_t& sigdata)
{
    m_rtc_cnt += sigdata.data.as_uint();
    if (!sigdata.index) return;

    if (m_next_rtc_event_type & TimerEventMax) {
        m_rtc_cnt = 0;
    }

    if (m_next_rtc_event_type & TimerEventPer) {
        m_rtc_cnt = 0;
        if (m_rtc_intflag.set_flag(RTC_OVF_bm))
            logger().dbg("RTC triggering OVF interrupt");
    }

    if (m_next_rtc_event_type & TimerEventCmp) {
        if (m_rtc_intflag.set_flag(RTC_CMP_bm))
            logger().dbg("RTC triggering CMP interrupt");
    }

    m_rtc_timer.set_timer_delay(rtc_delay());
}

void AVR_ArchMega0_RTC::pit_hook_raised(const signal_data_t& sigdata)
{
    m_pit_cnt += sigdata.data.as_uint();
    if (!sigdata.index) return;

    m_pit_cnt = 0;
    if (m_pit_intflag.set_flag(RTC_PI_bm))
        logger().dbg("PIT triggering interrupt");

    m_pit_timer.set_timer_delay(pit_delay());
}
