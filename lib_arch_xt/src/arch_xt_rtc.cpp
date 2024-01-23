/*
 * arch_xt_rtc.cpp
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

#include "arch_xt_rtc.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"
#include "core/sim_sleep.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================
//Macros definition to access the registers of the controller

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(RTC_t, reg))

#define REG_OFS(reg) \
    offsetof(RTC_t, reg)

typedef ArchXT_RTCConfig CFG;

#define PRESCALER_MAX 32678

//Enum values for the m_clk_mode variables;
enum RTC_Mode {
    RTC_Disabled = 0x00,
    RTC_Enabled = 0x01,
    PIT_Enabled = 0x02
};


//=======================================================================================

class ArchXT_RTC::TimerHook : public SignalHook {

public:

    explicit TimerHook(ArchXT_RTC& ctl) : m_ctl(ctl) {}

    virtual void raised(const signal_data_t& sigdata, int hooktag) override {
        if (hooktag)
            m_ctl.pit_hook_raised(sigdata);
        else
            m_ctl.rtc_hook_raised(sigdata);
    }

private:

    ArchXT_RTC& m_ctl;

};


ArchXT_RTC::ArchXT_RTC(const CFG& config)
:Peripheral(AVR_IOCTL_RTC)
,m_config(config)
,m_clk_mode(RTC_Disabled)
,m_rtc_counter(0x10000, 1)
,m_pit_counter(0x10000, 0)
,m_rtc_intflag(false)
,m_pit_intflag(false)
{
    m_timer_hook = new TimerHook(*this);
    m_rtc_counter.signal().connect(*m_timer_hook, 0);
    m_pit_counter.signal().connect(*m_timer_hook, 1);
}

ArchXT_RTC::~ArchXT_RTC()
{
    delete m_timer_hook;
}

bool ArchXT_RTC::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), RTC_RUNSTDBY_bm | RTC_PRESCALER_gm | RTC_CORREN_bm | RTC_RTCEN_bm);
    add_ioreg(REG_ADDR(STATUS), RTC_CMPBUSY_bm | RTC_PERBUSY_bm | RTC_CNTBUSY_bm | RTC_CTRLABUSY_bm, true);
    add_ioreg(REG_ADDR(INTCTRL), RTC_CMP_bm | RTC_OVF_bm);
    add_ioreg(REG_ADDR(INTFLAGS), RTC_CMP_bm | RTC_OVF_bm);
    add_ioreg(REG_ADDR(TEMP));
    //DBGCTRL not supported
    add_ioreg(REG_ADDR(CALIB));
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

    m_rtc_counter.init(*device.cycle_manager(), logger());
    m_pit_counter.init(*device.cycle_manager(), logger());

    return status;
}

void ArchXT_RTC::reset()
{
    m_clk_mode = RTC_Disabled;
    write_ioreg(REG_ADDR(PERL), 0xFF);
    write_ioreg(REG_ADDR(PERH), 0xFF);
    m_rtc_counter.reset();
    m_pit_counter.reset();
}

uint8_t ArchXT_RTC::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //16-bits reading of CNT
    if (reg_ofs == REG_OFS(CNTL)) {
        m_rtc_counter.update();
        uint16_t v = (uint16_t) m_rtc_counter.counter();
        value = v & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), v >> 8);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    //16-bits reading of PER
    else if (reg_ofs == REG_OFS(PERL)) {
        uint16_t v = (uint16_t) m_rtc_counter.top();
        value = v & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), v >> 8);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    //16-bits reading of CMP
    else if (reg_ofs == REG_OFS(CMPL)) {
        uint16_t v = (uint16_t) m_rtc_counter.comp_value(0);
        value = v & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), v >> 8);
    }
    else if (reg_ofs == REG_OFS(CMPH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    return value;
}

void ArchXT_RTC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    const reg_addr_t reg_ofs = addr - m_config.reg_base;
    bool do_reconfigure = false;

    if (reg_ofs == REG_OFS(CTRLA)) {
        if (TEST_IOREG(CTRLA, RTC_RTCEN))
            m_clk_mode |= RTC_Enabled;
        else
            m_clk_mode &= ~RTC_Enabled;

        do_reconfigure = (data.value != data.old);
    }

    else if (reg_ofs == REG_OFS(PITCTRLA)) {
        if (TEST_IOREG(PITCTRLA, RTC_PITEN)) {
            uint8_t period_index = READ_IOREG_F(PITCTRLA, RTC_PERIOD);
            if (period_index > 0 || period_index < 0xFF) {
                m_clk_mode |= PIT_Enabled;
                m_pit_counter.set_top((1 << (period_index + 1)) - 1);
            }
        } else {
            m_clk_mode &= ~PIT_Enabled;
        }

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
        m_rtc_counter.set_counter(temp | (data.value << 8));
        m_rtc_counter.reschedule();
    }

    //16-bits writing to PER
    else if (reg_ofs == REG_OFS(PERL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_rtc_counter.update();
        m_rtc_counter.set_top(temp | (data.value << 8));
        m_rtc_counter.reschedule();
    }

    //16-bits writing to CMP
    else if (reg_ofs == REG_OFS(CMPL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(CMPH)) {
        uint8_t temp = read_ioreg(REG_ADDR(TEMP));
        m_rtc_counter.update();
        m_rtc_counter.set_comp_value(0, temp | (data.value << 8));
        m_rtc_counter.reschedule();
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

    if (do_reconfigure)
        configure_timers();
}

void ArchXT_RTC::configure_timers()
{
    if (m_clk_mode) {
        //Read and configure the clock source
        uint8_t clk_mode_val = READ_IOREG_F(CLKSEL, RTC_CLKSEL);
        auto clksel_cfg = find_reg_config_p<CFG::clksel_config_t>(m_config.clocks, clk_mode_val);
        unsigned long clk_factor; //ratio (main MCU clock freq) / (RTC clock freq)
        if (clksel_cfg && clksel_cfg->source == CFG::Clock_32kHz)
            clk_factor = device()->frequency() / 32768;
        else if (clksel_cfg && clksel_cfg->source == CFG::Clock_1kHz)
            clk_factor = device()->frequency() / 1024;
        else {
            device()->crash(CRASH_INVALID_CONFIG, "Invalid RTC clock source");
            return;
        }

        //Read and configure the prescaler factor
        const unsigned long ps_max = PRESCALER_MAX * clk_factor;
        const unsigned long f = (1 << READ_IOREG_F(CTRLA, RTC_PRESCALER)) * clk_factor;
        m_rtc_counter.prescaler().set_prescaler(ps_max, f);
        m_pit_counter.prescaler().set_prescaler(ps_max, f);

        //Set the RTC and PIT counters mode
        m_rtc_counter.set_tick_source((m_clk_mode | RTC_Enabled) ? TimerCounter::Tick_Timer : TimerCounter::Tick_Stopped);
        m_pit_counter.set_tick_source((m_clk_mode | PIT_Enabled) ? TimerCounter::Tick_Timer : TimerCounter::Tick_Stopped);

    } else {

        m_rtc_counter.prescaler().set_prescaler(PRESCALER_MAX, 0);
        m_rtc_counter.prescaler().set_prescaler(PRESCALER_MAX, 0);

    }
}

/*
 *  Callback when entering sleep mode
 */
void ArchXT_RTC::sleep(bool on, SleepMode mode)
{
    //The RTC timer is paused for sleep modes above Standby and in Standby if RTC_RUNSTDBY is not set
    //The PIT timer is not affected by sleep modes
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !TEST_IOREG(CTRLA, RTC_RUNSTDBY)))
        m_rtc_counter.prescaler().set_paused(on);
}

/*
 *  Callbacks for the prescaled timers
 */
void ArchXT_RTC::rtc_hook_raised(const signal_data_t& sigdata)
{
    if (sigdata.sigid != TimerCounter::Signal_Event)
        return;

    int event_type = sigdata.data.as_int();

    if (event_type & TimerCounter::Event_Top) {
        if (m_rtc_intflag.set_flag(RTC_OVF_bm))
            logger().dbg("RTC triggering OVF interrupt");
    }

    if (event_type & TimerCounter::Event_Compare) {
        if (m_rtc_intflag.set_flag(RTC_CMP_bm))
            logger().dbg("RTC triggering CMP interrupt");
    }
}

void ArchXT_RTC::pit_hook_raised(const signal_data_t& sigdata)
{
    if (sigdata.sigid != TimerCounter::Signal_Event)
        return;

    if (sigdata.data.as_uint() & TimerCounter::Event_Top) {
        if (m_pit_intflag.set_flag(RTC_PI_bm))
            logger().dbg("PIT triggering interrupt");
    }
}
