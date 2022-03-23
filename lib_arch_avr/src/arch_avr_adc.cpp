/*
 * arch_avr_adc.cpp
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


#include "arch_avr_adc.h"
#include "arch_avr_extint.h"
#include "arch_avr_timer.h"
#include "core/sim_device.h"
#include "core/sim_sleep.h"


#define CFG AVR_ArchAVR_ADC_Config

static const uint32_t ADC_Prescaler_Max = 128;


//=======================================================================================

AVR_ArchAVR_ADC::AVR_ArchAVR_ADC(const AVR_ArchAVR_ADC_Config& config)
:AVR_Peripheral(AVR_IOCTL_ADC)
,m_config(config)
,m_state(ADC_Disabled)
,m_first(true)
,m_trigger_index(-1)
,m_temperature(25.0)
,m_latched_ch_mux(0)
,m_latched_ref_mux(0)
,m_conv_value(0)
,m_signal(NULL)
,m_intflag(true)
{}

AVR_ArchAVR_ADC::~AVR_ArchAVR_ADC()
{
	if (m_signal)
		delete m_signal;
}

bool AVR_ArchAVR_ADC::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	add_ioreg(m_config.reg_datal);
	add_ioreg(m_config.reg_datah);
	add_ioreg(m_config.rb_chan_mux);
	add_ioreg(m_config.rb_ref_mux);
	add_ioreg(m_config.rb_enable);
	add_ioreg(m_config.rb_start);
	add_ioreg(m_config.rb_auto_trig);
	add_ioreg(m_config.rb_int_enable);
	add_ioreg(m_config.rb_int_flag);
	add_ioreg(m_config.rb_prescaler);
	add_ioreg(m_config.rb_trig_mux);
	add_ioreg(m_config.rb_bipolar);
	add_ioreg(m_config.rb_left_adj);

	status &= m_intflag.init(device,
				   	   	     m_config.rb_int_enable,
							 m_config.rb_int_flag,
							 m_config.int_vector);

	status &= connect_trigger_signals();

	m_timer.init(device.cycle_manager(), device.logger());
	m_timer.signal().connect_hook(this, 0xFFFF);

	return status;
}

void AVR_ArchAVR_ADC::reset()
{
	m_state = ADC_Disabled;
	m_first = true;
	m_trigger_index = -1;
	m_conv_value = 0;
	m_timer.reset();
}

bool AVR_ArchAVR_ADC::ctlreq(uint16_t req, ctlreq_data_t* data)
{
	if (req == AVR_CTLREQ_GET_SIGNAL) {
		if (!m_signal)
			m_signal = new AVR_Signal;
		data->data = m_signal;
		return true;
	}
	else if (req == AVR_CTLREQ_ADC_SET_TEMP) {
		m_temperature = data->data.as_double();
		return true;
	}
	return false;
}


//=======================================================================================
//I/O register callback reimplementation

void AVR_ArchAVR_ADC::ioreg_read_handler(reg_addr_t addr)
{
	//The ADSC bit is dynamic, reading 1 if a conversion is in progress
	if (addr == m_config.rb_start.addr)
		write_ioreg(m_config.rb_start, (m_state > ADC_Idle ? 1 : 0));
}

void AVR_ArchAVR_ADC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	if (addr == m_config.rb_enable.addr) {
		//Positive edge on the enable bit (ADEN).
		//We reset the state and the prescaler and reconnect the trigger
		if (m_config.rb_enable.extract(data.posedge)) {
			m_state = ADC_Idle;
			m_first = true;
			reset_prescaler();
		}
		//Negative edge on the enable bit (ADEN).
		//We disable the ADC, stop the cycle timer (if a conversion is running) and discconnect the trigger
		else if (m_config.rb_enable.extract(data.negedge)) {
			if (m_state > ADC_Idle)
				m_timer.set_timer_delay(0);
			m_state = ADC_Disabled;
		}
	}

	if (addr == m_config.rb_start.addr) {
		//Writing a '1' to ADSC when it's idle starts a conversion cycle
		if (m_config.rb_start.extract(data.value) && m_state == ADC_Idle)
			start_conversion_cycle();
	}

	if (addr == m_config.rb_auto_trig.addr || addr == m_config.rb_trig_mux.addr)
		update_trigger_signal();

	if (addr == m_config.rb_left_adj.addr)
		write_digital_value();

	if (addr == m_config.rb_int_enable.addr)
		m_intflag.update_from_ioreg();

	//Writing 1 to ADIF clears the flag and cancels the interrupt
	if (addr == m_config.rb_int_flag.addr && m_config.rb_int_flag.extract(data.value))
		m_intflag.clear_flag();

}


//=======================================================================================
/*
 * Hook callback, the sigid determines if it's a timer signal or a trigger signal
 */
void AVR_ArchAVR_ADC::raised(const signal_data_t& sigdata, uint16_t hooktag)
{
	if (hooktag == 0xFFFF) {
		if (sigdata.index == 1)
			timer_raised();
	} else {
		trigger_raised(sigdata, hooktag);
	}
}


//=======================================================================================
//Conversion timing management

void AVR_ArchAVR_ADC::reset_prescaler()
{
	m_timer.reset();

	uint32_t clk_ps_factor = m_config.clk_ps_factors[read_ioreg(m_config.rb_prescaler)];
	m_timer.set_prescaler(ADC_Prescaler_Max, clk_ps_factor);
}

/*
 * Method that starts a conversion cycle
 */
void AVR_ArchAVR_ADC::start_conversion_cycle()
{
	DEBUG_LOG(device()->logger(), "ADC starting a conversion cycle", "");

	m_state = ADC_PendingConversion;

	//Backup the channel and reference mux values (as per the datasheet)
	m_latched_ch_mux = read_ioreg(m_config.rb_chan_mux);
	m_latched_ref_mux = read_ioreg(m_config.rb_ref_mux);

	//Number of cycle to do the conversion, including the time waiting for the first ADC clock tick
	int adc_ticks = 1 + (m_first) ? 13 : 2;

	//Start the prescaled timer
	m_timer.set_timer_delay(adc_ticks);

	//Raise the signal
	if (m_signal)
		m_signal->raise_u(0, m_latched_ch_mux, ADC_ConversionStarted);
}

/*
* Main function for reading and converting analog values
*/
#define _crash(text) \
	do { \
		device()->crash(CRASH_BAD_CTL_IO, text); \
		return; \
	} while(0);

void AVR_ArchAVR_ADC::read_analog_value()
{
	DEBUG_LOG(device()->logger(), "ADC reading analog value", "");

	//Find the channel mux configuration
	int index = find_reg_config<ADC_channel_config_t>(m_config.channels, m_latched_ch_mux);
	if (index == -1)
		_crash("ADC: Invalid channel configuration");
	auto ch_config = &(m_config.channels[index]);

	//Find the reference voltage mux configuration and request the value from the VREF peripheral
	float vref = 0.0;
	index = find_reg_config<CFG::reference_config_t>(m_config.references, m_latched_ref_mux);
	if(index == -1)
		_crash("ADC: Invalid reference configuration");
	auto ref_config = &(m_config.references[index]);
	ctlreq_data_t reqdata = { .index = ref_config->source };
	if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_ADC_GET_VREF, &reqdata))
		_crash("ADC: Unable to obtain the voltage reference");
	vref = reqdata.data.as_double();

	//Obtain the raw analog value depending on the channel mux configuration
	//The raw value is in the interval [0.0; 1.0] (or [-1.0; +1.0] for bipolar)
	//and is relative to VCC
	double raw_value;
	bool bipolar = false;
	switch(ch_config->type) {
		
		case ADC_SingleEnded: {
			AVR_Pin* p = device()->find_pin(ch_config->pin_p);
			if (!p) _crash("ADC: Invalid pin configuration");
			raw_value = p->analog_value();
		} break;

		case ADC_Differential: {
			AVR_Pin* p = device()->find_pin(ch_config->pin_p);
			if (!p) _crash("ADC: Invalid pin configuration");
			AVR_Pin* n = device()->find_pin(ch_config->pin_n);
			if (!n) _crash("ADC: Invalid pin configuration");
			raw_value = p->analog_value() - n->analog_value();
			bipolar = test_ioreg(m_config.rb_bipolar);
		} break;

		case ADC_BandGap: {
			ctlreq_data_t reqdata = { .index = AVR_IO_VREF::Source_Internal };
			if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_ADC_GET_VREF, &reqdata))
				_crash("ADC: Unable to obtain the band gap voltage value");
			raw_value = reqdata.data.as_double();
		} break;

		case ADC_Temperature: {
			double temp_volt = m_config.temp_cal_coef * (m_temperature - 25.0) + m_config.temp_cal_25C;
			//The temperature measure obtained is in absolute voltage values.
			//We need to make it relative to VCC
			ctlreq_data_t reqdata = { .index = AVR_IO_VREF::Source_Ext_VCC };
			if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_ADC_GET_VREF, &reqdata))
				_crash("ADC: Unable to obtain the VCC voltage value");
			raw_value = temp_volt / reqdata.data.as_double();
		} break;

		case ADC_Zero:
		default:
			raw_value = 0.0;
	}

	//Clip the raw analog value to the interval [-VCC; +VCC]
	if (raw_value < -1.0) raw_value = -1.0;
	if (raw_value > 1.0) raw_value = 1.0;

	//Convert the raw value to a 10-bits integer value with respect to VREF
	if (bipolar) {
		m_conv_value = int(raw_value * 512 / vref);
		if (m_conv_value > 511) m_conv_value = 511;
		if (m_conv_value < -512) m_conv_value = -512;
	} else {
		m_conv_value = int(raw_value * 1024 / vref);
		if (m_conv_value > 1023) m_conv_value = 1023;
		if (m_conv_value < 0) m_conv_value = 0;
	}
}

void AVR_ArchAVR_ADC::timer_raised()
{
	if (m_state == ADC_PendingConversion) {
		//Raise the signal
		if (m_signal)
			m_signal->raise_u(ADC_AboutToSample, m_latched_ch_mux, 0);

		read_analog_value();

		m_state = ADC_PendingRaise;

		//The next time this cycle timer is called is when the conversion
		//is complete (13 ADC clock ticks)
		m_timer.set_timer_delay(13);

	}

	else if (m_state == ADC_PendingRaise) {

		//Raise the signal
		if (m_signal)
			m_signal->raise_u(ADC_ConversionComplete, m_latched_ch_mux, 0);

		//Store the converted value in the data register according to the adjusting
		write_digital_value();

		m_state = ADC_Idle;
		m_first = false;

		if (m_intflag.set_flag())
			DEBUG_LOG(device()->logger(), "ADC interrupt triggered", "");

		//If free running auto-trigger is enabled, start a new conversion cycle
		if (m_trigger_index >= 0) {
			CFG::TriggerType trig_type = m_config.triggers[m_trigger_index].trig_type;
			if (trig_type == CFG::FreeRunning) {
				DEBUG_LOG(device()->logger(), "ADC in free running, starting a new conversion", "");
				start_conversion_cycle();
			}
		}
	}
}

/*
 * Method that stores the converted value in the data registers according to the
 * left adjust settings
 */
void AVR_ArchAVR_ADC::write_digital_value()
{
	uint8_t sign = (m_conv_value < 0 ? 1 : 0);
	uint16_t v = (m_conv_value < 0 ? -m_conv_value : m_conv_value);

	uint16_t r;
	if (test_ioreg(m_config.rb_left_adj))
		r = (sign << 15) | (v << 5);
	else
		r = (sign << 10) | v;

	DEBUG_LOG(device()->logger(), "ADC converted value: 0x%04x", r);

	write_ioreg(m_config.reg_datah, r >> 8);
	write_ioreg(m_config.reg_datal, r & 0x00FF);
}

//=============================================================================
//Management of auto triggers

bool AVR_ArchAVR_ADC::connect_trigger_signals()
{
	for (unsigned int i = 0; i < m_config.triggers.size(); ++i) {
		m_trigger_state.push_back(false);

		bool expect_signal = true;
		AVR_Signal* s;
		switch(m_config.triggers[i].trig_type) {
			case CFG::AComp:
				s = get_signal(AVR_IOCTL_ACOMP, 0);
				break;

			case CFG::ExtInt0:
				s = get_signal(AVR_IOCTL_EXTINT, 0);
				break;

			case CFG::Timer0_CompA:
				s = get_signal(AVR_IOCTL_TIMER('_', '0'), AVR_ArchAVR_Timer::Signal_CompA);
				break;

			case CFG::Timer0_OVF:
				s = get_signal(AVR_IOCTL_TIMER('_', '0'), AVR_ArchAVR_Timer::Signal_OVF);
				break;

			case CFG::Timer1_CompB:
				s = get_signal(AVR_IOCTL_TIMER('_', '1'), AVR_ArchAVR_Timer::Signal_CompB);
				break;

			case CFG::Timer1_OVF:
				s = get_signal(AVR_IOCTL_TIMER('_', '1'), AVR_ArchAVR_Timer::Signal_OVF);
				break;

			case CFG::Manual:
			case CFG::FreeRunning:
				expect_signal = false;
				s = nullptr;
				break;

			default:
				s = nullptr;
		}

		if (s) {
			s->connect_hook(this, i);
		}
		else if (expect_signal) {
			WARNING_LOG(device()->logger(),
					  	"ADC found no signal for trigger %x",
						m_config.triggers[i].reg_value);
		}
	}

	return true;
}


/* This method updates the trigger.
 * "Note that switching from a trigger source that is cleared to a trigger
 * source that is set, will generate a positive edge on the trigger signal." (AVR datasheet)
 * So we need to compare the state of the selected trigger before and after change.
 * If we have a positive edge, we force the start of a conversion cycle.
 */
void AVR_ArchAVR_ADC::update_trigger_signal()
{
	if (test_ioreg(m_config.rb_auto_trig)) {

		bool old_trig_state = (m_trigger_index >= 0) ? m_trigger_state[m_trigger_index] : false;

		uint8_t trig_reg_value = read_ioreg(m_config.rb_trig_mux);
		m_trigger_index = find_reg_config<CFG::trigger_config_t>(m_config.triggers, trig_reg_value);

		bool new_trig_state = (m_trigger_index >= 0) ? m_trigger_state[m_trigger_index] : false;

		if (new_trig_state && !old_trig_state) {
			reset_prescaler();
			start_conversion_cycle();
		}

	} else {
		m_trigger_index = -1;
	}
}

/*
 * Callback when the auto-trigger hook is raised
 */
void AVR_ArchAVR_ADC::trigger_raised(const signal_data_t& data, uint16_t sigid)
{
	//Obtain the corresponding trigger type
	CFG::TriggerType trig_type = m_config.triggers[sigid].trig_type;

	//Save the state of the signal, even if it's not the one selected
	//as trigger.
	//For ExtInt0, the signal is raised for any EXTINT but we can filter by index
	if (trig_type != CFG::ExtInt0 || data.index == 0)
		m_trigger_state[sigid] = data.data.as_uint();

	//No need to process further if:
	// - the signal raised is not the selected trigger, or
	// - The ADC is disabled or a conversion is already on-going, or
	// - Auto-trigger is disabled, or
	// - The device is in a sleep mode where the ADC is paused
	if (sigid != m_trigger_index ||
		m_state != ADC_Idle ||
		!test_ioreg(m_config.rb_auto_trig) ||
		device()->sleep_mode() > AVR_SleepMode::ADC) return;

	//Reset the prescaler and start a conversion
	reset_prescaler();
	start_conversion_cycle();
}

//=============================================================================
//Sleep management

/*
* The ADC is paused for modes above ADC Noise Reduction.
*/
void AVR_ArchAVR_ADC::sleep(bool on, AVR_SleepMode mode)
{
	if (mode > AVR_SleepMode::ADC) {
		if (on)
			DEBUG_LOG(device()->logger(), "ADC pausing", "");
		else
			DEBUG_LOG(device()->logger(), "ADC resuming", "");

		m_timer.set_paused(on);
	}
}
