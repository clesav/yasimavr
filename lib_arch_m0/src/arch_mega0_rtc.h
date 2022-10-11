/*
 * arch_mega0_rtc.h
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

#ifndef __YASIMAVR_MEGA0_RTC_H__
#define __YASIMAVR_MEGA0_RTC_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"


//=======================================================================================
/*
 * AVR_ArchMega0_RTC is the implementation of a Real-Time Clock for the Mega-0/Mega-1 series
 * This includes a Periodic Interrupt Timer system
 * Unsupported features:
 *      - External clock source
 *      - Crystal error correction
 *      - Events outputs
 *      - Synchronization Busy flags
 */

struct AVR_ArchMega0_RTC_Config {

    enum RTC_ClockSource {
        Clock_32kHz,
        Clock_1kHz
    };

    struct clksel_config_t : base_reg_config_t {
        RTC_ClockSource source;
    };

    std::vector<clksel_config_t> clocks;

    reg_addr_t reg_base;
    int_vect_t iv_rtc;
    int_vect_t iv_pit;

};


class DLL_EXPORT AVR_ArchMega0_RTC : public AVR_Peripheral {

public:

    AVR_ArchMega0_RTC(const AVR_ArchMega0_RTC_Config& config);
    virtual ~AVR_ArchMega0_RTC();

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual void ioreg_read_handler(reg_addr_t addr) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, AVR_SleepMode mode) override;

private:

    const AVR_ArchMega0_RTC_Config& m_config;

    uint8_t m_clk_mode;

    //***** Counters *****
    uint16_t m_rtc_cnt;     //RTC tick counter
    uint16_t m_rtc_per;     //RTC period
    uint16_t m_rtc_cmp;     //RTC compare register
    uint16_t m_pit_cnt;     //PIT tick counter

    //***** prescaled timer, one for each counter *****
    AVR_PrescaledTimer m_rtc_timer;
    AVR_PrescaledTimer m_pit_timer;
    uint8_t m_next_rtc_event_type;

    //***** Interrupt flags *****
    AVR_InterruptFlag m_rtc_intflag;
    AVR_InterruptFlag m_pit_intflag;

    //***** Timer signal hook implementation *****
    class TimerHook;
    friend class TimerHook;
    TimerHook* m_timer_hook;

    void rtc_hook_raised(const signal_data_t& sigdata);
    void pit_hook_raised(const signal_data_t& sigdata);

    void configure_timers();

    uint32_t rtc_delay();
    uint32_t pit_delay();

};

#endif
