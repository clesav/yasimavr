/*
 * arch_avr_adc.h
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_timer.h"
#include "ioctrl_common/sim_vref.h"


//=======================================================================================

struct AVR_ArchAVR_ADC_Config {
	
	struct reference_config_t : base_reg_config_t {
		AVR_IO_VREF::Source source;
	};
	
	enum TriggerType {
		Manual = 1,
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
		TriggerType trig_type;
	};
	
	std::vector<AVR_IO_ADC::channel_config_t> channels;
	std::vector<reference_config_t> references;
	std::vector<uint16_t> clk_ps_factors;
	std::vector<trigger_config_t> triggers;

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

	double temp_cal_25C;			//Temperature sensor value in V at +25°C
	double temp_cal_coef;			//Temperature sensor linear coef in V/°C
};


class DLL_EXPORT AVR_ArchAVR_ADC : public AVR_IO_ADC,
								   public AVR_Peripheral,
								   public AVR_SignalHook {

public:

	AVR_ArchAVR_ADC(const AVR_ArchAVR_ADC_Config& config);
	virtual ~AVR_ArchAVR_ADC();

	virtual bool init(AVR_Device& device) override;
	virtual void reset() override;
	virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
	virtual void ioreg_read_handler(reg_addr_t addr) override;
	virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
	virtual void sleep(bool on, AVR_SleepMode mode) override;
	virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override;

private:

	//enum defining the various steps of a conversion
	enum State {
		ADC_Disabled,
		ADC_Idle,
		ADC_PendingConversion,
		ADC_PendingRaise,
	};

    const AVR_ArchAVR_ADC_Config& m_config;
	
	//Step of the conversion
    State m_state;
	//Boolean defining if a conversion is the first after the ADC being enabled
	//It has impact on the timing
    bool m_first;

    //Timer to simulate the conversion cycle duration
    AVR_PrescaledTimer m_timer;

	//Trigger type configured from the registers
    int m_trigger_index;
    std::vector<bool> m_trigger_state;

	//Simulated device temperature value in deg Celsius
    double m_temperature;

	//Configuration values for the channel and reference latched at the start of a conversion
    uint8_t m_latched_ch_mux;
    uint8_t m_latched_ref_mux;

	//Raw converted value
    int16_t m_conv_value;

	//Signal raised at various steps of the conversion
    AVR_Signal* m_signal;

    //Interrupt flag raised at the completion of a conversion
    AVR_InterruptFlag m_intflag;

    void start_conversion_cycle();
    void reset_prescaler();
    void timer_raised();

    void read_analog_value();
    void write_digital_value();

    bool connect_trigger_signals();
    void update_trigger_signal();
    void trigger_raised(const signal_data_t& data, uint16_t sigid);

};

#endif //__YASIMAVR_AVR_ADC_H__
