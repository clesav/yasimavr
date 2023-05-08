/*
 * arch_xt_timer_b.h
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

#ifndef __YASIMAVR_XT_TIMER_B_H__
#define __YASIMAVR_XT_TIMER_B_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * ArchXT_TimerB is the implementation of a Timer/Counter type B for the XT core series
 * Only the Periodic Interrupt mode is currently implemented
 * Other unsupported features:
 *      - Event control and input
 *      - Debug run override
 *      - Compare/capture output on pin
 *      - Status register
 *      - Synchronize Update (SYNCUPD)
 */

struct ArchXT_TimerB_Config {

    reg_addr_t reg_base;
    int_vect_t iv_capt;

};

class DLL_EXPORT ArchXT_TimerB : public Peripheral, public SignalHook {

public:

    ArchXT_TimerB(int num, const ArchXT_TimerB_Config& config);

    //Override of Peripheral callbacks
    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;
    //Override of Hook callback
    virtual void raised(const signal_data_t& data, uint16_t sigid) override;

private:

    const ArchXT_TimerB_Config& m_config;

    uint8_t m_clk_mode;

    //***** Interrupt flag management *****
    InterruptFlag m_intflag;

    //***** Timer management *****
    PrescaledTimer m_timer;
    TimerCounter m_counter;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TIMER_B_H__
