/*
 * arch_mega0_adc.h
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


#ifndef __YASIMAVR_MEGA0_ADC_H__
#define __YASIMAVR_MEGA0_ADC_H__

#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_timer.h"
#include "ioctrl_common/sim_vref.h"


//=======================================================================================
/*
 * Implementation of a ADC for the Mega-0/Mega-1 series
 * Unsupported features:
 *      - Free running (TODO)
 *      - Sample Capacitance Selection
 *      - Automatic Sampling Delay Variation
 *      - Event Control
 *      - Debug Run Override
 *      - Duty Cycle calibration
 *
 * Note: Without the event control system, only the software can manually start a conversion.
 * As a workaround, a conversion can be triggered externally by the simulation environment
 * by using the request AVR_CTLREQ_ADC_FORCE_TRIGGER.
 */

struct AVR_ArchMega0_ADC_Config {

    struct reference_config_t : base_reg_config_t {
        AVR_IO_VREF::Source source;
    };

    std::vector<AVR_IO_ADC::channel_config_t> channels;
    std::vector<reference_config_t> references;
    std::vector<uint16_t> clk_ps_factors;
    uint16_t clk_ps_max;
    std::vector<uint16_t> init_delays;

    reg_addr_t reg_base;

    int_vect_t iv_resready;
    int_vect_t iv_wincmp;

    double temp_cal_25C;            //Temperature sensor value in V at +25 degC
    double temp_cal_coef;           //Temperature sensor linear coef in V/degC

};

class DLL_EXPORT AVR_ArchMega0_ADC : public AVR_IO_ADC,
                                     public AVR_Peripheral,
                                     public AVR_SignalHook {

public:

    AVR_ArchMega0_ADC(const AVR_ArchMega0_ADC_Config& config);

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_read_handler(reg_addr_t addr) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, AVR_SleepMode mode) override;
    virtual void raised(const signal_data_t& data, uint16_t sigid) override;

private:

    enum State {
        ADC_Disabled,
        ADC_Idle,
        ADC_Starting,
        ADC_PendingConversion,
        ADC_PendingRaise,
    };

    const AVR_ArchMega0_ADC_Config& m_config;
    State m_state;
    bool m_first;
    AVR_PrescaledTimer m_timer;
    double m_temperature;
    uint8_t m_latched_ch_mux;
    uint8_t m_latched_ref_mux;
    uint8_t m_accum_counter;
    uint16_t m_result;
    uint16_t m_win_lothres;
    uint16_t m_win_hithres;
    AVR_InterruptFlag m_res_intflag;
    AVR_InterruptFlag m_cmp_intflag;
    AVR_Signal m_signal;

    void start_conversion_cycle();
    void read_analog_value();

};

#endif //__YASIMAVR_MEGA0_ADC_H__
