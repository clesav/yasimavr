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

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

#define AVR_CTLREQ_TCB_GET_EVENT_HOOK         1

/**
   \ingroup api_timer
   \brief Configuration structure for ArchXT_TimerB.
 */
struct ArchXT_TimerBConfig {

    enum Options {
        EventCount   = 0x01,
        OverflowFlag = 0x02,
    };

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for TCB_CAPT
    int_vect_t iv_capt = AVR_INTERRUPT_NONE;
    int options = 0x00;

};

/**
   \ingroup api_timer
   \brief Implementation of a Timer/Counter type B for the XT core series

   Unsupported features:
   - Debug run override
   - Synchronize Update (SYNCUPD)
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_TimerB : public Peripheral, public SignalHook {

public:

    enum SignalId {
        Signal_Capture,
        Signal_Output,
    };

    enum CaptureHookTag {
        Tag_Event,
        Tag_Count,
    };

    ArchXT_TimerB(int num, const ArchXT_TimerBConfig& config);
    virtual ~ArchXT_TimerB();

    //Override of Peripheral callbacks
    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;
    //Override of Hook callback
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    class EventHook;
    friend class EventHook;

    enum State {
        State_Ready,
        State_Run,
        State_End
    };

    const ArchXT_TimerBConfig& m_config;

    int m_clk_mode;
    int m_cnt_mode;
    State m_cnt_state;
    uint16_t m_ccmp;
    bool m_event_state;
    uint8_t m_output;

    //***** Interrupt flag management *****
    InterruptFlag m_intflag;

    //***** Timer management *****
    TimerCounter m_counter;

    EventHook* m_event_hook;

    DataSignal m_signal;

    void set_counter_state(State state);
    void update_counter_top();
    void update_on_CCMP_read();
    void process_capture_event(unsigned char event_state);
    void hook_raised(const signal_data_t& data, int hooktag);
    void process_count_event();
    void raise_capture_flag();
    void update_output(int change);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TIMER_B_H__
