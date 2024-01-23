/*
 * arch_xt_rtc.h
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

#ifndef __YASIMAVR_XT_RTC_H__
#define __YASIMAVR_XT_RTC_H__

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Configuration structure for ArchXT_RTC.
 */
struct ArchXT_RTCConfig {

    enum RTC_ClockSource {
        Clock_32kHz,
        Clock_1kHz
    };

    /**
       \brief Configuration structure for each supported source clock
       \param reg_value the register field value for this source
       \param source RTC_ClockSource enum value
     */
    struct clksel_config_t : base_reg_config_t {
        RTC_ClockSource source;
    };

    /// Clock source configurations
    std::vector<clksel_config_t> clocks;
    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for RTC
    int_vect_t iv_rtc;
    /// Interrupt vector index for PIT
    int_vect_t iv_pit;

};

/**
   \brief Implementation of a RTC controller for XT core series

   Includes a Periodic Interrupt Timer system

   Unsupported features:
     - External clock source
     - Crystal error correction
     - Events outputs
     - Synchronization Busy flags
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_RTC : public Peripheral {

public:

    explicit ArchXT_RTC(const ArchXT_RTCConfig& config);
    virtual ~ArchXT_RTC();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;

private:

    const ArchXT_RTCConfig& m_config;

    int m_clk_mode;

    //***** prescaled timer, one for each counter *****
    TimerCounter m_rtc_counter;
    TimerCounter m_pit_counter;

    //***** Interrupt flags *****
    InterruptFlag m_rtc_intflag;
    InterruptFlag m_pit_intflag;

    //***** Timer signal hook implementation *****
    class TimerHook;
    friend class TimerHook;
    TimerHook* m_timer_hook;

    void rtc_hook_raised(const signal_data_t& sigdata);
    void pit_hook_raised(const signal_data_t& sigdata);

    void configure_timers();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_RTC_H__
