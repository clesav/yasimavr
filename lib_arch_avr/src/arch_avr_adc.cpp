/*
 * arch_avr_adc.cpp
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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
#include <cmath>

YASIMAVR_USING_NAMESPACE

using namespace ADC;


//=======================================================================================

#define CFG ArchAVR_ADCConfig

static const uint32_t ADC_PrescalerMax = 128;


ArchAVR_ADC::ArchAVR_ADC(int num, const CFG& config)
:Peripheral(AVR_IOCTL_ADC(0x30 + num))
,m_config(config)
,m_state(ADC_Disabled)
,m_first(true)
,m_trigger(CFG::Trig_Manual)
,m_timer_hook(*this, &ArchAVR_ADC::timer_raised)
,m_latched_ch_mux(0)
,m_latched_ref_mux(0)
,m_conv_value(0)
,m_intflag(true)
{}


bool ArchAVR_ADC::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rbc_result);
    add_ioreg(m_config.rb_chan_mux);
    add_ioreg(m_config.rb_ref_mux);
    add_ioreg(m_config.rb_enable);
    add_ioreg(m_config.rb_start);
    add_ioreg(m_config.rb_auto_trig);
    add_ioreg(m_config.rb_int_enable);
    add_ioreg(m_config.rb_int_flag, IORegister::Strobe);
    add_ioreg(m_config.rb_prescaler);
    add_ioreg(m_config.rb_trig_mux);
    add_ioreg(m_config.rb_bipolar);
    add_ioreg(m_config.rb_left_adj);

    status &= m_intflag.init(device,
                             m_config.rb_int_enable,
                             m_config.rb_int_flag,
                             m_config.iv_adc);

    m_timer.init(*device.cycle_manager(), logger());
    m_timer.signal().connect(m_timer_hook);

    return status;
}


void ArchAVR_ADC::reset(int)
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
    if (addr == m_config.rb_start)
        value = m_config.rb_start.replace(value, (m_state > ADC_Idle ? 1 : 0));

    return value;
}


void ArchAVR_ADC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.rb_enable) {
        //Positive edge on the enable bit (ADEN).
        //We reset the state and the prescaler and reconnect the trigger
        if (m_config.rb_enable.extract(data.posedge())) {
            //Check that the main voltage is set. If not, crash the device
            VREF::getset_t vref_reqinfo = VREF::getset_t{ .source = VREF::Source_VCC };
            ctlreq_data_t vref_reqdata = { .data = &vref_reqinfo };
            if (device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &vref_reqdata)) {
                m_state = ADC_Idle;
                m_first = true;
                reset_prescaler();
            }
        }
        //Negative edge on the enable bit (ADEN).
        //We disable the ADC, stop the cycle timer (if a conversion is running) and discconnect the trigger
        else if (m_config.rb_enable.extract(data.negedge())) {
            if (m_state > ADC_Idle)
                m_timer.set_timer_delay(0);
            m_state = ADC_Disabled;
        }
    }

    if (addr == m_config.rb_start) {
        //Writing a '1' to ADSC when it's idle starts a conversion cycle
        if (m_config.rb_start.extract(data.value) && m_state == ADC_Idle)
            start_conversion_cycle();
    }

    if (addr == m_config.rb_auto_trig || addr == m_config.rb_trig_mux) {
        if (test_ioreg(m_config.rb_auto_trig)) {
            uint8_t trig_reg_value = read_ioreg(m_config.rb_trig_mux);
            auto trig_cfg = find_reg_config_p(m_config.triggers, trig_reg_value);
            m_trigger = trig_cfg ? trig_cfg->trigger : CFG::Trig_Manual;
        } else {
            m_trigger = CFG::Trig_Manual;
        }
    }

    if (addr == m_config.rb_left_adj)
        store_converted_value();

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
    if (m_config.rb_ref_mux.valid())
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
    auto ch_config = find_reg_config_p(m_config.channels, m_latched_ch_mux);
    if (!ch_config)
        _crash("ADC: Invalid channel configuration");

    double vref;
    if (m_config.rb_ref_mux.valid()) {
        //Find the reference voltage mux configuration and request the value from the VREF peripheral
        auto ref_config = find_reg_config_p(m_config.references, m_latched_ref_mux);
        if(!ref_config)
            _crash("ADC: Invalid reference configuration");

        VREF::getset_t vref_reqinfo = VREF::getset_t{ .source = ref_config->source };
        ctlreq_data_t vref_reqdata = { .data = &vref_reqinfo };
        if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &vref_reqdata))
            _crash("ADC: Unable to obtain the voltage reference");
        vref = vref_reqinfo.voltage;
    } else {
        //If the ADC has no ref mux, the reference is assumed to be VCC
        vref = 1.0;
    }

    //Obtain the raw analog value depending on the channel mux configuration
    //The raw value is in the interval [0.0; 1.0] (or [-1.0; +1.0] for bipolar)
    //and is relative to VCC
    double raw_value;
    bool bipolar = false;
    switch(ch_config->type) {

        case Channel_SingleEnded: {
            Pin* p = device()->find_pin(ch_config->pin_p);
            if (!p) _crash("ADC: Invalid pin configuration");

            m_signal.raise(Signal_AboutToSamplePin, p->id());

            raw_value = p->voltage();
        } break;

        case Channel_Differential: {
            Pin* p = device()->find_pin(ch_config->pin_p);
            if (!p) _crash("ADC: Invalid pin configuration");
            Pin* n = device()->find_pin(ch_config->pin_n);
            if (!n) _crash("ADC: Invalid pin configuration");

            m_signal.raise(Signal_AboutToSamplePin, p->id());
            m_signal.raise(Signal_AboutToSamplePin, n->id());

            raw_value = p->voltage() - n->voltage();

            bipolar = m_config.rb_bipolar.valid() && test_ioreg(m_config.rb_bipolar);
        } break;

        case Channel_IntRef: {
            VREF::getset_t vref_reqinfo = VREF::getset_t{ .source = VREF::Source_Mux, .channel = m_config.vref_channel };
            ctlreq_data_t vref_reqdata = { .data = &vref_reqinfo };
            device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &vref_reqdata);
            raw_value = vref_reqinfo.voltage;
        } break;

        case Channel_Temperature: {
            m_signal.raise(Signal_AboutToSampleTemp);
            VREF::getset_t vref_reqinfo = VREF::getset_t{ .source = VREF::Source_Temperature };
            ctlreq_data_t vref_reqdata = { .data = &vref_reqinfo };
            device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &vref_reqdata);
            raw_value = vref_reqinfo.voltage;
        } break;

        case Channel_Zero:
        default:
            raw_value = 0.0;
    }

    //Applies the channel gain
    raw_value *= ch_config->gain;
    //Make the final value relative to VREF
    raw_value /= vref;
    //Constraint the raw analog value to the interval [-1.0; +1.0]
    if (raw_value < -1.0) raw_value = -1.0;
    if (raw_value > 1.0) raw_value = 1.0;

    //Convert the raw value to an integer value with respect to bipolar/unipolar mode
    int32_t int_range, int_min;
    if (bipolar) {
        int_range = 1 << (m_config.result_width - 1);
        int_min = -int_range;
    } else {
        int_range = 1 << m_config.result_width;
        int_min = 0;
    }

    int32_t int_result = lrint(raw_value * int_range);
    if (int_result >= int_range)
        int_result = int_range - 1;
    if (int_result < int_min)
        int_result = int_min;

    m_conv_value = int_result;
}


void ArchAVR_ADC::timer_raised(const signal_data_t& sigdata, int)
{
    if (sigdata.index != 1) return;

    if (m_state == ADC_PendingConversion) {
        read_analog_value();

        m_state = ADC_PendingRaise;

        //The next time this cycle timer is called is when the conversion
        //is complete (13 ADC clock ticks)
        m_timer.set_timer_delay(13);
    }

    else if (m_state == ADC_PendingRaise) {

        //Raise the signal
        m_signal.raise(Signal_ConversionComplete);

        //Store the converted value in the data register according to the adjusting
        store_converted_value();

        m_state = ADC_Idle;
        m_first = false;

        m_intflag.set_flag();

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
void ArchAVR_ADC::store_converted_value()
{
    int32_t adj_result;
    if (m_config.rb_left_adj.valid() && test_ioreg(m_config.rb_left_adj))
        adj_result = m_conv_value << (m_config.rbc_result.bitcount() - m_config.result_width);
    else
        adj_result = m_conv_value;

    logger().dbg("Converted value: 0x%04x", adj_result);

    write_ioreg(m_config.rbc_result, adj_result);
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
