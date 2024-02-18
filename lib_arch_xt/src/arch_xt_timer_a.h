/*
 * arch_xt_timer_a.h
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

#ifndef __YASIMAVR_XT_TIMER_A_H__
#define __YASIMAVR_XT_TIMER_A_H__

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

YASIMAVR_BEGIN_NAMESPACE

class ArchXT_TimerB;


//=======================================================================================

/**
   \file
   \name Controller requests definition for ArchXT_TimerA
   @{
 */

/*
 * Definition of CTLREQ codes for Timer type A
 */

/**
   \ingroup api_timer
   Request for a Timer type B to register with the timer type A. this is necessary
   for the TCB to use the same "clock" as the TCA.
 */
#define AVR_CTLREQ_TCA_REGISTER_TCB         1

/// @}

/**
   \ingroup api_timer
   Request for the event input hook for a timer type A.
   It returns with data as a pointer to a SignalHook instance.
 */
#define AVR_CTLREQ_TCA_GET_EVENT_HOOK      2


//=======================================================================================

/**
   \ingroup api_timer
   \brief Configuration structure for ArchXT_TimerA.
 */
struct ArchXT_TimerAConfig {

    /// Defines the number of comparison channels supported by the TCA
    static const int CompareChannelCount = 3;

    enum Version {
        /// Base model version, corresponding to ATMega 0-series implementations
        V1,
        /// V1 + Event Input B and RUNSTDBY bit features
        V2
    };

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for TCA_OVF (a.k.a. TCA_LUNF)
    int_vect_t iv_ovf;
    /// Interrupt vector index for TCA_HUNF
    int_vect_t iv_hunf;
    /// Array of vector index for the compare channels interrupts
    int_vect_t ivs_cmp[CompareChannelCount];
    /// Version of the model
    Version version = V1;

};

/**
   \ingroup api_timer
   \brief Implementation of a Timer/Counter type A for the XT core series.

   Unsupported features:
        - Debug run override

  The model supports two versions, defined by the `version`attribute of ArchXT_TimerAConfig
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_TimerA : public Peripheral, public SignalHook {

public:

    enum SignalId {
        Signal_CompareOutput
    };

    enum EventHookTag {
        Hook_EventA,
        Hook_EventB
    };

    explicit ArchXT_TimerA(const ArchXT_TimerAConfig& config);
    virtual ~ArchXT_TimerA();

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

    struct buffered_reg_t {
        uint16_t value = 0;
        uint16_t buffer = 0;
        bool flag = false;
    };

    enum CounterHookTag {
        Tag_Single,
        Tag_SplitLow,
        Tag_SplitHigh
    };

    class EventHook;
    friend class EventHook;

    const ArchXT_TimerAConfig& m_config;

    //***** Counters *****
    buffered_reg_t m_per;
    buffered_reg_t m_cmp[ArchXT_TimerAConfig::CompareChannelCount];

    //***** Interrupt and signal management *****
    InterruptFlag m_ovf_intflag;
    InterruptFlag m_hunf_intflag;
    InterruptFlag m_cmp_intflags[ArchXT_TimerAConfig::CompareChannelCount];

    DataSignal m_signal;

    //***** Counter management *****
    bool m_split_mode;
    PrescaledTimer m_timer;
    TimerCounter m_sgl_counter; //Main counter in Single mode
    TimerCounter m_lo_counter; //Low-byte counter in Split mode
    TimerCounter m_hi_counter; //High-byte counter in Split mode

    uint8_t m_wgmode;

    bool m_EIA_state;
    bool m_EIB_state;
    bool m_timer_block;

    EventHook* m_event_hook;

    uint8_t read_ioreg_single(reg_addr_t reg_ofs, uint8_t value);
    uint8_t read_ioreg_split(reg_addr_t reg_ofs, uint8_t value);
    void write_ioreg_single(reg_addr_t reg_ofs, const ioreg_write_t& data);
    void write_ioreg_split(reg_addr_t reg_ofs, const ioreg_write_t& data);
    void update_ALUPD_status();
    void update_buffered_registers();
    void set_peripheral_mode(bool split_mode);
    void update_tick_sources();
    void configure_single_counter();
    void update_timer_block(uint8_t ev_ctrl);
    void set_direction(bool countdown, bool do_reschedule);
    void set_compare_output(int index, int change);
    void update_compare_outputs(int change = 0);

    void process_counter_single(const signal_data_t& sigdata);
    void process_counter_split(const signal_data_t& sigdata, bool low_cnt);

    void event_raised(const signal_data_t& sigdata, int hooktag);
    void process_EIA(bool event_state);
    void process_EIB(bool event_state);

    bool execute_command(int cmd, bool cmden);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TIMER_A_H__
