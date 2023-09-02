/*
 * arch_avr_adc.h
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

#ifndef __YASIMAVR_AVR_ADC_H__
#define __YASIMAVR_AVR_ADC_H__

#include "arch_avr_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_timer.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

struct ArchAVR_ADCConfig {

    struct reference_config_t : base_reg_config_t {
        VREF::Source source;
    };

    enum Trigger {
        Trig_Manual,
        Trig_FreeRunning,
        Trig_External,
    };

    struct trigger_config_t : base_reg_config_t {
        Trigger trigger;
    };

    std::vector<ADC::channel_config_t> channels;
    std::vector<reference_config_t> references;
    std::vector<unsigned long> clk_ps_factors;
    std::vector<trigger_config_t> triggers;

    unsigned int vref_channel;

    reg_addr_t reg_datal;
    reg_addr_t reg_datah;

    regbit_t rb_chan_mux;
    regbit_t rb_ref_mux;
    regbit_t rb_enable;
    regbit_t rb_start;
    regbit_t rb_auto_trig;
    regbit_t rb_int_enable;
    regbit_t rb_int_flag;
    regbit_t rb_prescaler;
    regbit_t rb_trig_mux;
    regbit_t rb_bipolar;
    regbit_t rb_left_adj;

    int_vect_t int_vector;

    double temp_cal_25C;            //Temperature sensor value in V at +25°C
    double temp_cal_coef;           //Temperature sensor linear coef in V/°C
};


class AVR_ARCHAVR_PUBLIC_API ArchAVR_ADC : public ADC,
                                           public Peripheral,
                                           public SignalHook {

public:

    ArchAVR_ADC(int num, const ArchAVR_ADCConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    //enum defining the various steps of a conversion
    enum State {
        ADC_Disabled,
        ADC_Idle,
        ADC_PendingConversion,
        ADC_PendingRaise,
    };

    const ArchAVR_ADCConfig& m_config;

    //Step of the conversion
    State m_state;
    //Boolean defining if a conversion is the first after the ADC being enabled
    //It has impact on the timing
    bool m_first;

    ArchAVR_ADCConfig::Trigger m_trigger;

    //Timer to simulate the conversion cycle duration
    PrescaledTimer m_timer;

    //Simulated device temperature value in deg Celsius
    double m_temperature;

    //Configuration values for the channel and reference latched at the start of a conversion
    uint8_t m_latched_ch_mux;
    uint8_t m_latched_ref_mux;

    //Raw converted value
    int16_t m_conv_value;

    //Signal raised at various steps of the conversion
    Signal m_signal;

    //Interrupt flag raised at the completion of a conversion
    InterruptFlag m_intflag;

    void start_conversion_cycle();
    void reset_prescaler();
    void timer_raised();

    void read_analog_value();
    void write_digital_value();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_ADC_H__
