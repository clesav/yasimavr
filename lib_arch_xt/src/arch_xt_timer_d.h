/*
 * arch_xt_timer_d.h
 *
 *  Copyright 2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_XT_TIMER_D_H__
#define __YASIMAVR_XT_TIMER_D_H__

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_timer
   \brief Configuration structure for ArchXT_TimerD.
 */
struct ArchXT_TimerDConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for TCD_OVF
    int_vect_t iv_ovf;

};

/**
   \ingroup api_timer
   \brief Implementation of a Timer/Counter type D for the XT core series.

   Basic model limited to generating an overflow interrupt periodically.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_TimerD : public Peripheral {

public:

    explicit ArchXT_TimerD(const ArchXT_TimerDConfig& config);

    //Override of Peripheral callbacks
    virtual bool init(Device& device) override;
    virtual void reset(int flags) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const ArchXT_TimerDConfig& m_config;

    //***** Interrupt and signal management *****
    InterruptFlag m_ovf_intflag;

    //***** Counter management *****
    TimerCounter m_counter;

    uint8_t m_wgmode;
    unsigned int m_cycle_state;
    BoundFunctionSignalHook<ArchXT_TimerD> m_counter_hook;

    void start_counter();
    void stop_counter();
    void configure_counter();
    void configure_counter_cycle_state();

    void counter_raised(const signal_data_t& sigdata, int hooktag);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TIMER_D_H__
