/*
 * arch_avr_wdt.h
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_AVR_WDT_H__
#define __YASIMAVR_AVR_WDT_H__

#include "arch_avr_globals.h"
#include "core/sim_cycle_timer.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE

//=======================================================================================

/**
   \brief Configuration structure for ArchAVR_WDT
 */
struct ArchAVR_WDTConfig {

    /// Clock frequency used by the watchdog timer
    unsigned long clock_frequency;
    /// List of selectable delays
    std::vector<unsigned long> delays;
    /// WDT configuration register address
    reg_addr_t reg_wdt;
    /// Bitmask for the delay select
    regbit_compound_t rbc_delay;
    /// Bitmask for the Change Enable bit
    bitmask_t bm_chg_enable;
    /// Bitmask for the Reset Enable bit
    bitmask_t bm_reset_enable;
    /// Bitmask for the Interrupt Enable bit
    bitmask_t bm_int_enable;
    /// Bitmask for the Interrupt Flag bit
    bitmask_t bm_int_flag;
    /// Regbit for the reset flag
    regbit_t rb_reset_flag;
    /// Interrupt vector index
    int_vect_t iv_wdt;

};


/**
   \brief Implementation of a Watchdog Timer for AVR series
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_WDT : public Peripheral, public InterruptHandler {

public:

    explicit ArchAVR_WDT(const ArchAVR_WDTConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void interrupt_ack_handler(int_vect_t vector) override;

private:

    const ArchAVR_WDTConfig& m_config;
    cycle_count_t m_timer_start_cycle;
    BoundFunctionCycleTimer<ArchAVR_WDT> m_wdt_timer;
    BoundFunctionCycleTimer<ArchAVR_WDT> m_lock_timer;

    void reschedule_timer();
    cycle_count_t calculate_timeout_delay();
    cycle_count_t wdt_timeout(cycle_count_t when);
    void lock_timeout();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_WDT_H__
