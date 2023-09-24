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
   Defines the number of comparison channels supported by the TCA
 */
#define AVR_TCA_CMP_CHANNEL_COUNT           3


//=======================================================================================

/**
   \ingroup api_timer
   \brief Configuration structure for ArchXT_TimerA.
 */
struct ArchXT_TimerAConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for TCA_OVF
    int_vect_t iv_ovf;
    /// List of interrupt vector indexes for TCB_CMPx
    int_vect_t ivs_cmp[AVR_TCA_CMP_CHANNEL_COUNT];

};

/**
   \ingroup api_timer
   \brief Implementation of a Timer/Counter type A for the XT core series.

   Only the Normal WGM mode in Single timer configuration is currently implemented.
   Other unsupported features:
        - Event control and input
        - Debug run override
        - Compare/capture output on pin
        - Lock Update
        - Timer command for forced update/restart

   CTLREQs supported:
    - AVR_CTLREQ_TCA_REGISTER_TCB : Used internally by ArchXT_TimerB instances to
      link their prescaler clock source to the TCA prescaler output
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_TimerA : public Peripheral, public SignalHook {

public:

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

    const ArchXT_TimerAConfig& m_config;

    //***** Clock management *****
    std::vector<ArchXT_TimerB*> m_synched_timers;

    //***** Counters *****
    uint16_t m_cnt;                                 //Current counter register value
    uint16_t m_per;                                 //Current period register value
    uint16_t m_cmp[AVR_TCA_CMP_CHANNEL_COUNT];      //Current compare registers values
    uint16_t m_perbuf;                              //Period buffer register
    uint16_t m_cmpbuf[AVR_TCA_CMP_CHANNEL_COUNT];   //Compare buffer registers

    //***** Event management *****
    uint8_t m_next_event_type;

    //***** Interrupt and signal management *****
    InterruptFlag m_ovf_intflag;
    InterruptFlag* m_cmp_intflags[AVR_TCA_CMP_CHANNEL_COUNT];

    //***** Counter management *****
    PrescaledTimer m_timer;

    uint32_t delay_to_event();
    void update_16bits_buffers();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TIMER_A_H__
