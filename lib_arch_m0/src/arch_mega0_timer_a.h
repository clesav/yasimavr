/*
 * arch_mega0_timer_a.h
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

#ifndef __YASIMAVR_MEGA0_TIMER_A_H__
#define __YASIMAVR_MEGA0_TIMER_A_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

class AVR_ArchMega0_TimerB;


//=======================================================================================
/*
 * Definition of CTLREQ codes for Timer type A
 */

//Request for a Timer type B to register with the timer type A. this is necessary
//for the TCB to use the same "clock" as the TCA.
#define AVR_CTLREQ_TCA_REGISTER_TCB         1


//Defines the number of comparison channels supported by the TCA
#define AVR_TCA_CMP_CHANNEL_COUNT           3


//=======================================================================================
/*
 * Implementation of a Timer/Counter type A for the Mega-0/Mega-1 series
 * Only the Normal WGM mode in Single timer configuration is currently implemented
 * Other unsupported features:
 *      - Event control and input
 *      - Debug run override
 *      - Compare/capture output on pin
 *      - Lock Update
 *      - Timer command for forced update/restart
 */

struct AVR_ArchMega0_TimerA_Config {

    reg_addr_t reg_base;

    int_vect_t iv_ovf;
    int_vect_t ivs_cmp[AVR_TCA_CMP_CHANNEL_COUNT];

};

class DLL_EXPORT AVR_ArchMega0_TimerA : public AVR_Peripheral, public AVR_SignalHook {

public:

    AVR_ArchMega0_TimerA(const AVR_ArchMega0_TimerA_Config& config);

    //Override of AVR_Peripheral callbacks
    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_read_handler(reg_addr_t addr) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, AVR_SleepMode mode) override;
    //Override of Hook callback
    virtual void raised(const signal_data_t& sigdata, uint16_t sigid) override;

private:

    const AVR_ArchMega0_TimerA_Config& m_config;

    //***** Clock management *****
    std::vector<AVR_ArchMega0_TimerB*> m_synched_timers;

    //***** Counters *****
    uint16_t m_cnt;                                 //Current counter register value
    uint16_t m_per;                                 //Current period register value
    uint16_t m_cmp[AVR_TCA_CMP_CHANNEL_COUNT];      //Current compare registers values
    uint16_t m_perbuf;                              //Period buffer register
    uint16_t m_cmpbuf[AVR_TCA_CMP_CHANNEL_COUNT];   //Compare buffer registers

    //***** Event management *****
    uint8_t m_next_event_type;

    //***** Interrupt and signal management *****
    AVR_InterruptFlag m_ovf_intflag;
    AVR_InterruptFlag m_cmp_intflags[AVR_TCA_CMP_CHANNEL_COUNT];

    //***** Counter management *****
    AVR_PrescaledTimer m_timer;

    uint32_t delay_to_event();
    void update_16bits_buffers();

};


#endif
