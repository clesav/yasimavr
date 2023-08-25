/*
 * arch_avr_timer.h
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

#ifndef __YASIMAVR_AVR_TIMER_H__
#define __YASIMAVR_AVR_TIMER_H__

#include "arch_avr_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_timer.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Implementation of a 8bits/16bits Timer/Counter for AVR series
 * Other unsupported features:
 *      - Compare/capture output on pin
 *      - Asynchronous operations
 *
 * This timer is a flexible implementation aiming at covering most modes found in
 * AVR timer/counter. It covers normal, CTC, PWM in both single and dual slopes.
 * The behaviour is defined by a mode_config_t structure selected by the mode field.
 * It has a number of Output Compare channels, each defined by a OC_config_t structure.
 * Each OC channel behaviour is defined by a set of Compare Output Mode (COM) values.
 *
 */

/*
 * CTLREQ definitions
*/
//Request to obtain a pointer to the SignalHook entry point for external clock ticks
#define AVR_CTLREQ_TMR_GET_EXTCLK_HOOK        1
//Request to obtain a pointer to the SignalHook entry point for event capture
#define AVR_CTLREQ_TMR_GET_CAPT_HOOK          2


struct ArchAVR_TimerConfig {

    enum COM {
        COM_NoChange = 0,
        COM_Toggle,
        COM_Clear,
        COM_Set,
        COM_ToggleA,
    };

    enum Top {
        Top_OnMax = 0,
        Top_OnFixed,
        Top_OnCompA,
        Top_OnIC,
    };

    enum OCR {
        OCR_Unbuffered = 0,
        OCR_UpdateOnTop,
        OCR_UpdateOnBottom,
    };

    enum OVF {
        OVF_SetOnMax = 0,
        OVF_SetOnTop,
        OVF_SetOnBottom,
    };

    struct clock_config_t : base_reg_config_t {
        TimerCounter::TickSource source;      //Clock source
        unsigned long div;                    //Prescaler factor
    };

    struct vector_config_t {
        int_vect_t num;
        unsigned char bit;
    };

    struct OC_config_t : base_reg_config_t {
        reg_addr_t reg_oc;
        vector_config_t vector;
        regbit_t rb_mode;
        regbit_t rb_force;
    };

    struct COM_config_t : base_reg_config_t {
        COM up : 4;
        COM down : 4;
        COM bottom : 4;
        COM top : 4;
    };

    struct mode_config_t : base_reg_config_t {
        //Controls when the OVerFlow interrupt flag is set
        OVF ovf : 2;
        //Controls the counter value used for TOP
        Top top : 2;
        //Controls the fixed top value when top is set to Top_OnFixed.
        //The fixed value is (2^n - 1), where n = (fixed_top_exp + 8)
        unsigned int fixed_top_exp : 4;
        //Controls when the OC compare values are updated from the registers
        OCR ocr : 2;
        //Controls the slope mode , false=single, true=double
        bool double_slope : 1;
        //If true, a Forced Output Compare strobe has no effect
        bool disable_foc: 1;
        //Controls which COM config variant is used
        unsigned int com_variant : 4;
    };

    typedef std::vector<COM_config_t> COM_variant_t;


    bool is_16bits;

    std::vector<clock_config_t> clocks;     //List of clock source configurations
    std::vector<mode_config_t> modes;       //List of the timer mode configurations

    /*
     * List of COM enum values
     * The COM values are presented in a 2D vector, indexed by:
     *  - a variant index, selected by the timer mode (in mode_config_t)
     *  - a OC mode selected by the OC channel mode field (in OC_config_t)
     */
    std::vector<COM_variant_t> com_modes;

    std::vector<OC_config_t> oc_channels;   //List of Output Compare channel configurations

    regbit_t rb_clock;                      //Clock/prescaler configuration register
    regbit_compound_t rbc_mode;             //Timer mode control register
    reg_addr_t reg_cnt;                     //Counter register
    reg_addr_t reg_icr;                     //Input compare register
    reg_addr_t reg_int_enable;              //Interrupt enable register
    reg_addr_t reg_int_flag;                //Interrupt flag register

    vector_config_t vect_ovf;               //Overflow Interrupt configuration
    vector_config_t vect_icr;               //Input Capture Interrupt configuration

};


class AVR_ARCHAVR_PUBLIC_API ArchAVR_Timer : public Peripheral, public SignalHook {

public:

    enum SignalId {
        //Signal raised on a overflow event, no data is carried
        Signal_OVF,
        //Signal raised on a Compare Match event. The index indicates which channel
        //(0='A', 1='B', etc...), no data is carried.
        Signal_CompMatch,
        //Signal raised with the Compare Output state.
        //The index indicates which channel (0='A', 1='B', etc...)
        //The data is the state (0 or 1) or invalid data if the channel is disabled.
        Signal_CompOutput,
        //Signal raised on a Input Capture event, no data is carried.
        Signal_Capt
    };

    ArchAVR_Timer(int num, const ArchAVR_TimerConfig& config);
    ~ArchAVR_Timer();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    class CaptureHook;
    friend class CaptureHook;

    struct OutputCompareChannel;
    friend struct OutputCompareChannel;

    const ArchAVR_TimerConfig& m_config;

    //***** Clock management *****
    unsigned long m_clk_ps_max;              //Max value counted by the clock prescaler
    //Input Capture Register value
    uint16_t m_icr;
    //Temporary register when the CPU is reading 16-bits registers
    uint8_t m_temp;
    //Current timer/counter mode
    ArchAVR_TimerConfig::mode_config_t m_mode;
    //List of output compare modules
    std::vector<OutputCompareChannel*> m_oc_channels;
    //Event timer engine
    PrescaledTimer m_timer;
    //Timer counter engine
    TimerCounter m_counter;
    //Interrupt and signal management
    InterruptFlag m_intflag_ovf;
    InterruptFlag m_intflag_icr;
    DataSignal m_signal;
    CaptureHook* m_capt_hook;

    void update_top();
    void capt_raised();
    ArchAVR_TimerConfig::COM_config_t get_COM_config(uint8_t regval);
    void change_OC_state(size_t index, int event_flags);
    bool output_active(ArchAVR_TimerConfig::COM_config_t& mode, size_t output_index);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_TIMER_H__
