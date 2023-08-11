/*
 * arch_avr_adc.sip
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

struct ArchAVR_ADCConfig {
%TypeHeaderCode
#include "arch_avr_adc.h"
%End

    struct reference_config_t : base_reg_config_t {
        VREF::Source source;
    };

    enum Trigger {
        Trig_Manual         /PyName=Manual/,
        Trig_FreeRunning    /PyName=FreeRunning/,
        Trig_External       /PyName=External/,
    };

    struct trigger_config_t : base_reg_config_t {
        ArchAVR_ADCConfig::Trigger trigger;
    };

    std::vector<ADC::channel_config_t> channels;
    std::vector<ArchAVR_ADCConfig::reference_config_t> references;
    UnsignedLongVector clk_ps_factors;
    std::vector<ArchAVR_ADCConfig::trigger_config_t> triggers;

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

    double temp_cal_25C;
    double temp_cal_coef;
};


class ArchAVR_ADC : public ADC, public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_adc.h"
%End

public:

    ArchAVR_ADC(int, const ArchAVR_ADCConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, SleepMode);
    virtual void raised(const signal_data_t&, int);

};
