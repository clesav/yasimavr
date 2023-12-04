/*
 * arch_xt_adc.cpp
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


#include "arch_xt_adc.h"
#include "arch_xt_acp.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_sleep.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(ADC_t, reg))

#define REG_OFS(reg) \
    reg_addr_t(offsetof(ADC_t, reg))

#define CFG ArchXT_ADCConfig

static const uint32_t ADC_Prescaler_Max = 256;


ArchXT_ADC::ArchXT_ADC(int num, const CFG& config)
:Peripheral(AVR_IOCTL_ADC(0x30 + num))
,m_config(config)
,m_state(ADC_Disabled)
,m_first(false)
,m_temperature(25.0)
,m_latched_ch_mux(0)
,m_latched_ref_mux(0)
,m_accum_counter(0)
,m_result(0)
,m_win_lothres(0)
,m_win_hithres(0)
,m_res_intflag(false)
,m_cmp_intflag(false)
{}

bool ArchXT_ADC::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), ADC_RUNSTBY_bm | ADC_RESSEL_bm | ADC_FREERUN_bm | ADC_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), ADC_SAMPNUM_gm);
    add_ioreg(REG_ADDR(CTRLC), ADC_PRESC_gm | ADC_REFSEL_gm | ADC_SAMPCAP_bm);
    add_ioreg(REG_ADDR(CTRLD), ADC_INITDLY_gm | ADC_ASDV_bm | ADC_SAMPDLY_gm);
    add_ioreg(REG_ADDR(CTRLE), ADC_WINCM_gm);
    add_ioreg(REG_ADDR(SAMPCTRL), ADC_SAMPLEN_gm);
    add_ioreg(REG_ADDR(MUXPOS), ADC_MUXPOS_gm);
    add_ioreg(REG_ADDR(COMMAND), ADC_STCONV_bm);
    add_ioreg(REG_ADDR(EVCTRL), ADC_STARTEI_bm);
    add_ioreg(REG_ADDR(INTCTRL), ADC_WCMP_bm | ADC_RESRDY_bm);
    add_ioreg(REG_ADDR(INTFLAGS), ADC_WCMP_bm | ADC_RESRDY_bm);
    //DBGCTRL not implemented
    add_ioreg(REG_ADDR(TEMP));
    add_ioreg(REG_ADDR(RESL));
    add_ioreg(REG_ADDR(RESH));
    add_ioreg(REG_ADDR(WINLTL));
    add_ioreg(REG_ADDR(WINLTH));
    add_ioreg(REG_ADDR(WINHTL));
    add_ioreg(REG_ADDR(WINHTH));
    add_ioreg(REG_ADDR(CALIB), ADC_DUTYCYC_bm);

    status &= m_res_intflag.init(device,
                                 DEF_REGBIT_B(INTCTRL, ADC_RESRDY),
                                 DEF_REGBIT_B(INTFLAGS, ADC_RESRDY),
                                 m_config.iv_resready);
    status &= m_cmp_intflag.init(device,
                                 DEF_REGBIT_B(INTCTRL, ADC_WCMP),
                                 DEF_REGBIT_B(INTFLAGS, ADC_WCMP),
                                 m_config.iv_wincmp);

    m_timer.init(*device.cycle_manager(), logger());
    m_timer.signal().connect(*this);

    return status;
}

void ArchXT_ADC::reset()
{
    m_state = ADC_Disabled;
    m_result = 0;
    m_win_lothres = 0;
    m_win_hithres = 0;
    m_timer.reset();
}

bool ArchXT_ADC::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
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
        if (m_state == ADC_Idle && TEST_IOREG(EVCTRL, ADC_STARTEI))
            start_conversion_cycle();
        return true;
    }
    return false;
}


//I/O register callback reimplementation

uint8_t ArchXT_ADC::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    //The STCONV bit is dynamic, reading 1 if a conversion is in progress
    if (addr == REG_ADDR(COMMAND))
        value = DEF_BITMASK_B(ADC_STCONV).replace(value, (m_state > ADC_Idle ? 1 : 0));

    return value;
}

void ArchXT_ADC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        //the enable bit or the refsel field might change.
        m_first = true;

        //Positive edge on the enable bit (CTRLA.ENABLE).
        //We reset the state and the prescaler
        if (data.posedge() & ADC_ENABLE_bm) {
            m_state = ADC_Idle;
        }
        //Negative edge on the enable bit (CTRLA.ENABLE).
        //We disable the ADC, stop the cycle timer (if a conversion is running)
        else if (data.negedge() & ADC_ENABLE_bm) {
            if (m_state > ADC_Idle)
                m_timer.set_timer_delay(0);
            m_state = ADC_Disabled;
        }
    }

    else if (reg_ofs == REG_OFS(COMMAND)) {
        //Writing a '1' to STCONV when it's idle starts a conversion cycle
        if ((data.value & ADC_STCONV_bm) && m_state == ADC_Idle)
            start_conversion_cycle();
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_res_intflag.update_from_ioreg();
        m_cmp_intflag.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        if (data.value && ADC_RESRDY_bm)
            m_res_intflag.clear_flag();

        if (data.value && ADC_WCMP_bm)
            m_cmp_intflag.clear_flag();
    }
}


/*
 * Method that starts a conversion cycle
 */
void ArchXT_ADC::start_conversion_cycle()
{
    logger().dbg("Starting a conversion cycle");

    m_state = ADC_Starting;
    m_accum_counter = 0;

    //Backup the channel and reference mux values (as per the datasheet)
    m_latched_ch_mux = READ_IOREG_F(MUXPOS, ADC_MUXPOS);
    m_latched_ref_mux = READ_IOREG_F(CTRLC, ADC_REFSEL);

    //Number of cycles before the actual start of the conversion
    uint8_t ps_setting = READ_IOREG_F(CTRLC, ADC_PRESC);
    uint16_t ps_factor = m_config.clk_ps_factors[ps_setting];
    uint32_t ps_start_ticks = (ps_factor/ 2) + 2;

    //Reset, setup and start the prescaled timer
    m_timer.reset();
    m_timer.set_prescaler(ADC_Prescaler_Max, 1);
    m_timer.set_timer_delay(ps_start_ticks);

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

void ArchXT_ADC::read_analog_value()
{
    logger().dbg("Reading analog value");

    //Find the channel mux configuration
    auto ch_config = find_reg_config_p<channel_config_t>(m_config.channels, m_latched_ch_mux);
    if (!ch_config)
        _crash("ADC: Invalid channel configuration");

    //Find the reference voltage mux configuration and request the value from the VREF peripheral
    double vref = 0.0;
    auto ref_config = find_reg_config_p<CFG::reference_config_t>(m_config.references, m_latched_ref_mux);
    if (!ref_config)
        _crash("ADC: Invalid reference configuration");

    ctlreq_data_t reqdata = { .data = m_config.vref_channel, .index = ref_config->source };
    if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata))
        _crash("ADC: Unable to obtain the voltage reference");
    vref = reqdata.data.as_double();
    if (vref == 0.0)
        _crash("ADC: Zero voltage reference");

    //Obtain the raw analog value depending on the channel mux configuration
    //The raw value is in the interval [0.0; 1.0] (or [-1.0; +1.0] for bipolar)
    //and is relative to VCC
    double raw_value;
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
        } break;

        case Channel_IntRef: {
            ctlreq_data_t reqdata = { .data = m_config.vref_channel,
                                      .index = VREF::Source_Internal };
            if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata))
                _crash("ADC: Unable to obtain the internal reference voltage value");
            raw_value = reqdata.data.as_double();
        } break;

        case Channel_AcompRef: {
            ctlreq_data_t reqdata;
            if (!device()->ctlreq(AVR_IOCTL_ACP(ch_config->per_num), AVR_CTLREQ_ACP_GET_DAC, &reqdata))
                _crash("ADC: Unable to obtain the DAC reference from the Analog Comparator");
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

    //Convert the raw value to a 10-bits integer value with respect to VREF
    uint16_t result = (uint16_t) int(raw_value * 1023 / vref);
    if (result > 1023) result = 1023;
    if (result < 0) result = 0;

    //Reduce the resolution to 8 bits if enabled
    if (TEST_IOREG(CTRLA, ADC_RESSEL))
        result >>= 2;

    if (!m_accum_counter)
        m_result = result;
    else
        m_result += result;
}

/*
 * Callback from the timer hook.
 * We arrive here twice in a conversion cycle.
 * First, we perform the actual analog read.
 * Second, we store it in the data register and raise the interrupt flag
 */
void ArchXT_ADC::raised(const signal_data_t& sigdata, int)
{
    if (sigdata.index != 1) return;

    if (m_state == ADC_Starting) {

        m_state = ADC_PendingConversion;

        //Number of cycles to perform the sample
        uint32_t adc_ticks = 2 + READ_IOREG_F(CTRLD, ADC_SAMPDLY);

        //If this is the first sample, we add the initial delay
        if (m_first) {
            uint8_t init_delay_setting = READ_IOREG_F(CTRLD, ADC_INITDLY);
            adc_ticks += m_config.init_delays[init_delay_setting];
            m_first = false;
        }

        //Reset, setup and start the prescaled timer
        uint8_t ps_setting = READ_IOREG_F(CTRLC, ADC_PRESC);
        uint16_t ps_factor = m_config.clk_ps_factors[ps_setting];
        m_timer.reset();
        m_timer.set_prescaler(ADC_Prescaler_Max, ps_factor);
        m_timer.set_timer_delay(adc_ticks);
    }

    else if (m_state == ADC_PendingConversion) {

        //Raise the signal
        m_signal.raise(Signal_AboutToSample, m_latched_ch_mux);

        //Do the sampling
        read_analog_value();

        m_state = ADC_PendingRaise;

        //Nb ADC clock ticks to complete the conversion 11 + extra length of sampling
        uint32_t adc_ticks = 11 + READ_IOREG_F(SAMPCTRL, ADC_SAMPLEN);
        m_timer.set_timer_delay(adc_ticks);
    }

    else if (m_state == ADC_PendingRaise) {

        //Raise the signal
        m_signal.raise(Signal_ConversionComplete, m_latched_ch_mux);

        //If we need to accumulate more samples, we return to PendingConversion state
        //and recall the cycle timer after the sampling delay
        if (++m_accum_counter < (1 << READ_IOREG_F(CTRLB, ADC_SAMPNUM))) {
            m_state = ADC_PendingConversion;
            m_timer.set_timer_delay(2 + READ_IOREG_F(CTRLD, ADC_SAMPDLY));
            return;
        }

        logger().dbg("Conversion complete");

        //Store the result
        WRITE_IOREG(RESL, m_result & 0xFF);
        WRITE_IOREG(RESH, (m_result >> 8) & 0xFF);

        m_state = ADC_Idle;

        //If the interrupt flag is not already set, we raise it
        //If also the interrupt is enabled, we raise it
        if (m_res_intflag.set_flag())
            logger().dbg("Triggering RESREADY interrupt");

        uint8_t winmode = READ_IOREG_F(CTRLE, ADC_WINCM);
        bool raise_win_int;
        switch(winmode) {
        case ADC_WINCM_BELOW_gc:
            raise_win_int = (m_result < m_win_lothres); break;
        case ADC_WINCM_ABOVE_gc:
            raise_win_int = (m_result > m_win_hithres); break;
        case ADC_WINCM_INSIDE_gc:
            raise_win_int = (m_result > m_win_lothres && m_result < m_win_hithres); break;
        case ADC_WINCM_OUTSIDE_gc:
            raise_win_int = (m_result < m_win_lothres || m_result > m_win_hithres); break;
        default:
            raise_win_int = false;
        }

        if (raise_win_int) {
            if (m_cmp_intflag.set_flag())
                logger().dbg("Triggering WINCOMP interrupt");
        }

        //if Automatic Sampling Delay Variation is enabled, increment the delay bits
        if (TEST_IOREG(CTRLD, ADC_ASDV)) {
            uint8_t dly = READ_IOREG_F(CTRLD, ADC_INITDLY);
            dly = (dly + 1) % 16;
            WRITE_IOREG_F(CTRLD, ADC_INITDLY, dly);
        }

        //If free run mode is enabled, start immediately another conversion cycle
        if (TEST_IOREG(CTRLA, ADC_FREERUN))
            start_conversion_cycle();
    }
}

//=============================================================================
//Sleep management

/*
 * The ADC is paused for modes above Standby and in Standby if RUNSTBY is not set
 */
void ArchXT_ADC::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !TEST_IOREG(CTRLA, ADC_RUNSTBY))) {
        if (on)
            logger().dbg("Pausing");
        else
            logger().dbg("Resuming");

        m_timer.set_paused(on);
    }
}
