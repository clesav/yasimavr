/*
 * arch_avr_adc.cpp
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

#include "arch_avr_adc.h"
#include "core/sim_sleep.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define CFG ArchAVR_ADCConfig

static const uint32_t ADC_PrescalerMax = 128;


ArchAVR_ADC::ArchAVR_ADC(int num, const CFG& config)
:Peripheral(AVR_IOCTL_ADC(0x30 + num))
,m_config(config)
,m_state(ADC_Disabled)
,m_first(true)
,m_trigger(CFG::Trig_Manual)
,m_temperature(25.0)
,m_latched_ch_mux(0)
,m_latched_ref_mux(0)
,m_conv_value(0)
,m_intflag(true)
{}


bool ArchAVR_ADC::init(Device& device)
{
    bool status = Peripheral::init(device);

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

    m_timer.init(*device.cycle_manager(), logger());
    m_timer.signal().connect(*this);

    return status;
}

void ArchAVR_ADC::reset()
{
    m_state = ADC_Disabled;
    m_first = true;
    m_trigger = CFG::Trig_Manual;
    m_conv_value = 0;
    m_timer.reset();
}

bool ArchAVR_ADC::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_ADC_SET_TEMP) {
        m_temperature = data->data.as_double();
        return true;
    }
    else if (req == AVR_CTLREQ_ADC_TRIGGER) {
        if (m_state == ADC_Idle && m_trigger == CFG::Trig_External) {
            reset_prescaler();
            start_conversion_cycle();
        }
        return true;
    }
    return false;
}


//=======================================================================================
//I/O register callback reimplementation

uint8_t ArchAVR_ADC::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    //The ADSC bit is dynamic, reading 1 if a conversion is in progress
    if (addr == m_config.rb_start.addr)
        value = m_config.rb_start.replace(value, (m_state > ADC_Idle ? 1 : 0));

    return value;
}

void ArchAVR_ADC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.rb_enable.addr) {
        //Positive edge on the enable bit (ADEN).
        //We reset the state and the prescaler and reconnect the trigger
        if (m_config.rb_enable.extract(data.posedge())) {
            m_state = ADC_Idle;
            m_first = true;
            reset_prescaler();
        }
        //Negative edge on the enable bit (ADEN).
        //We disable the ADC, stop the cycle timer (if a conversion is running) and discconnect the trigger
        else if (m_config.rb_enable.extract(data.negedge())) {
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

    if (addr == m_config.rb_auto_trig.addr || addr == m_config.rb_trig_mux.addr) {
        if (test_ioreg(m_config.rb_auto_trig)) {
            uint8_t trig_reg_value = read_ioreg(m_config.rb_trig_mux);
            auto trig_cfg = find_reg_config_p<CFG::trigger_config_t>(m_config.triggers, trig_reg_value);
            m_trigger = trig_cfg ? trig_cfg->trigger : CFG::Trig_Manual;
        } else {
            m_trigger = CFG::Trig_Manual;
        }
    }

    if (addr == m_config.rb_left_adj.addr)
        write_digital_value();

    if (addr == m_config.rb_int_enable.addr)
        m_intflag.update_from_ioreg();

    //Writing 1 to ADIF clears the flag and cancels the interrupt
    if (addr == m_config.rb_int_flag.addr && m_config.rb_int_flag.extract(data.value))
        m_intflag.clear_flag();

}


//=======================================================================================
//Conversion timing management

void ArchAVR_ADC::reset_prescaler()
{
    m_timer.reset();

    uint32_t clk_ps_factor = m_config.clk_ps_factors[read_ioreg(m_config.rb_prescaler)];
    m_timer.set_prescaler(ADC_PrescalerMax, clk_ps_factor);
}

/*
 * Method that starts a conversion cycle
 */
void ArchAVR_ADC::start_conversion_cycle()
{
    logger().dbg("Starting a conversion cycle");

    m_state = ADC_PendingConversion;

    //Backup the channel and reference mux values (as per the datasheet)
    m_latched_ch_mux = read_ioreg(m_config.rb_chan_mux);
    m_latched_ref_mux = read_ioreg(m_config.rb_ref_mux);

    //Number of cycle to do the conversion, including the time waiting for the first ADC clock tick
    int adc_ticks = 1 + (m_first) ? 13 : 2;

    //Start the prescaled timer
    m_timer.set_timer_delay(adc_ticks);

    //Raise the signal
    m_signal.raise(Signal_ConversionStarted, m_latched_ch_mux);
}

/*
* Main function for reading and converting analog values
*/
#define _crash(text) \
    do { \
        device()->crash(CRASH_BAD_CTL_IO, text); \
        return; \
    } while(0);

void ArchAVR_ADC::read_analog_value()
{
    logger().dbg("Reading analog value");

    //Find the channel mux configuration
    auto ch_config = find_reg_config_p<channel_config_t>(m_config.channels, m_latched_ch_mux);
    if (!ch_config)
        _crash("ADC: Invalid channel configuration");

    //Find the reference voltage mux configuration and request the value from the VREF peripheral
    auto ref_config = find_reg_config_p<CFG::reference_config_t>(m_config.references, m_latched_ref_mux);
    if(!ref_config)
        _crash("ADC: Invalid reference configuration");

    ctlreq_data_t reqdata = { .index = ref_config->source };
    if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata))
        _crash("ADC: Unable to obtain the voltage reference");
    double vref = reqdata.data.as_double();

    //Obtain the raw analog value depending on the channel mux configuration
    //The raw value is in the interval [0.0; 1.0] (or [-1.0; +1.0] for bipolar)
    //and is relative to VCC
    double raw_value;
    bool bipolar = false;
    switch(ch_config->type) {

        case Channel_SingleEnded: {
            Pin* p = device()->find_pin(ch_config->pin_p);
            if (!p) _crash("ADC: Invalid pin configuration");
            raw_value = p->voltage();
        } break;

        case Channel_Differential: {
            Pin* p = device()->find_pin(ch_config->pin_p);
            if (!p) _crash("ADC: Invalid pin configuration");
            Pin* n = device()->find_pin(ch_config->pin_n);
            if (!n) _crash("ADC: Invalid pin configuration");
            raw_value = p->voltage() - n->voltage();
            bipolar = test_ioreg(m_config.rb_bipolar);
        } break;

        case Channel_IntRef: {
            ctlreq_data_t reqdata = { .data = m_config.vref_channel, .index = ref_config->source };
            if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata))
                _crash("ADC: Unable to obtain the band gap voltage value");
            raw_value = reqdata.data.as_double();
        } break;

        case Channel_Temperature: {
            double temp_volt = m_config.temp_cal_coef * (m_temperature - 25.0) + m_config.temp_cal_25C;
            //The temperature measure obtained is in absolute voltage values.
            //We need to make it relative to VCC
            ctlreq_data_t reqdata = { .index = VREF::Source_VCC };
            if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata))
                _crash("ADC: Unable to obtain the VCC voltage value");
            raw_value = temp_volt / reqdata.data.as_double();
        } break;

        case Channel_Zero:
        default:
            raw_value = 0.0;
    }

    //Clip the raw analog value to the interval [-VCC; +VCC]
    if (raw_value < -1.0) raw_value = -1.0;
    if (raw_value > 1.0) raw_value = 1.0;

    //Applies the channel configuration gain
    raw_value *= ch_config->gain;

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


void ArchAVR_ADC::raised(const signal_data_t& sigdata, int)
{
    if (sigdata.index != 1) return;

    if (m_state == ADC_PendingConversion) {
        //Raise the signal
        m_signal.raise(Signal_AboutToSample, m_latched_ch_mux);

        read_analog_value();

        m_state = ADC_PendingRaise;

        //The next time this cycle timer is called is when the conversion
        //is complete (13 ADC clock ticks)
        m_timer.set_timer_delay(13);

    }

    else if (m_state == ADC_PendingRaise) {

        //Raise the signal
        m_signal.raise(Signal_ConversionComplete, m_latched_ch_mux);

        //Store the converted value in the data register according to the adjusting
        write_digital_value();

        m_state = ADC_Idle;
        m_first = false;

        if (m_intflag.set_flag())
            logger().dbg("Interrupt triggered");

        //If free running auto-trigger is enabled, start a new conversion cycle
        if (m_trigger == CFG::Trig_FreeRunning) {
            logger().dbg("In free running, starting a new conversion");
            start_conversion_cycle();
        }
    }
}

/*
 * Method that stores the converted value in the data registers according to the
 * left adjust settings
 */
void ArchAVR_ADC::write_digital_value()
{
    uint8_t sign = (m_conv_value < 0 ? 1 : 0);
    uint16_t v = (m_conv_value < 0 ? -m_conv_value : m_conv_value);

    uint16_t r;
    if (test_ioreg(m_config.rb_left_adj))
        r = (sign << 15) | (v << 5);
    else
        r = (sign << 10) | v;

    logger().dbg("Converted value: 0x%04x", r);

    write_ioreg(m_config.reg_datah, r >> 8);
    write_ioreg(m_config.reg_datal, r & 0x00FF);
}


//=============================================================================
//Sleep management

/*
* The ADC is paused for modes above ADC Noise Reduction.
*/
void ArchAVR_ADC::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::ADC) {
        if (on)
            logger().dbg("Pausing");
        else
            logger().dbg("Resuming");

        m_timer.set_paused(on);
    }
}
