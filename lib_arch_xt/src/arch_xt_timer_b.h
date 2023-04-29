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


//=======================================================================================
/*
 * AVR_ArchXT_TimerB is the implementation of a Timer/Counter type B for the XT core series
 * Only the Periodic Interrupt mode is currently implemented
 * Other unsupported features:
 *      - Event control and input
 *      - Debug run override
 *      - Compare/capture output on pin
 *      - Status register
 *      - Synchronize Update (SYNCUPD)
 */

struct AVR_ArchXT_TimerB_Config {

    reg_addr_t reg_base;
    int_vect_t iv_capt;

};

class DLL_EXPORT AVR_ArchXT_TimerB : public AVR_Peripheral, public AVR_SignalHook {

public:

    AVR_ArchXT_TimerB(int num, const AVR_ArchXT_TimerB_Config& config);

    //Override of AVR_Peripheral callbacks
    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, AVR_SleepMode mode) override;
    //Override of Hook callback
    virtual void raised(const signal_data_t& data, uint16_t sigid) override;

private:

    const AVR_ArchXT_TimerB_Config& m_config;

    uint8_t m_clk_mode;

    //***** Interrupt flag management *****
    AVR_InterruptFlag m_intflag;

    //***** Timer management *****
    AVR_PrescaledTimer m_timer;
    AVR_TimerCounter m_counter;

};

#endif
