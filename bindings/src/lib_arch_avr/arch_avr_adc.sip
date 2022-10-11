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

struct AVR_ArchAVR_ADC_Config {
%TypeHeaderCode
#include "arch_avr_adc.h"
%End

    struct reference_config_t : base_reg_config_t {
        AVR_IO_VREF::Source source;
    };

    enum TriggerType {
        Manual,
        FreeRunning,
        AComp,
        ExtInt0,
        Timer0_CompA,
        Timer0_OVF,
        Timer1_CompB,
        Timer1_OVF,
        //Timer1_Capt, //Not supported
    };

    struct trigger_config_t : base_reg_config_t {
        AVR_ArchAVR_ADC_Config::TriggerType trig_type;
    };

    std::vector<AVR_IO_ADC::channel_config_t> channels;
    std::vector<AVR_ArchAVR_ADC_Config::reference_config_t> references;
    UnsignedInt16Vector clk_ps_factors;
    std::vector<AVR_ArchAVR_ADC_Config::trigger_config_t> triggers;

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


class AVR_ArchAVR_ADC : public AVR_IO_ADC,
                        public AVR_Peripheral,
                        public AVR_SignalHook
                        /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_adc.h"
%End

public:

    AVR_ArchAVR_ADC(const AVR_ArchAVR_ADC_Config&);
    virtual ~AVR_ArchAVR_ADC();

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual void ioreg_read_handler(reg_addr_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, AVR_SleepMode);
    virtual void raised(const signal_data_t&, uint16_t);

};
