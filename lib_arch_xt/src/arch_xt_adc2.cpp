/*
 * arch_xt_adc2.cpp
 *
 *  Copyright 2026 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_xt_adc2.h"
#include "avr_io/io_adc2.h"
#include "arch_xt_acp.h"
#include "core/sim_sleep.h"
#include <cmath>
#include <algorithm>

YASIMAVR_USING_NAMESPACE

using namespace ADC;


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(ADC_t, reg))

#define REG_OFS(reg) \
    reg_addr_t(offsetof(ADC_t, reg))


//Constant values of initialisation delay (in us)
#define INIT_DELAY_ADC_MIN            10.0      //Min delay for ADC init in us
#define INIT_DELAY_ADC_CYCLES         20        //Min delay for ADC init in cycles
#define INIT_DELAY_PGA                20.0
#define INIT_DELAY_REF                60.0
#define INIT_DELAY_TEMP               35.0
#define INIT_DELAY_DAC                35.0

//Flags indicated if a module needs initialisation delay
#define INIT_FLAG_ADC 0x01
#define INIT_FLAG_REF 0x02
#define INIT_FLAG_PGA 0x04
#define INIT_FLAG_ALL INIT_FLAG_ADC | INIT_FLAG_REF | INIT_FLAG_PGA


//Clock prescaler factor, they are half of the actual settings because our timer
//counts half ADC cycles.
static const unsigned long ClockPrescalerFactors[] = {
    1, 2, 3, 4,
    5, 6, 7, 8,
    10, 12, 14, 16,
    20, 24, 28, 32
};

//PGA sample durations in half ADC cycles
static const unsigned long PGASampleDelays[] = { 12, 30, 40, 40 /*last value is placeholder*/ };

struct pga_gain_config_t : base_reg_config_t {
    double gain;
};

//PGA sample gains
static const auto PGAGains = std::to_array<pga_gain_config_t>({
    { 0x00, 1.0 },
    { 0x01, 2.0 },
    { 0x02, 4.0 },
    { 0x03, 8.0 },
    { 0x04, 16.0 },
});


#define _CRASH(text) \
    do { \
        device()->crash(CRASH_BAD_CTL_IO, text); \
        return; \
    } while(0);


#define TEST_VERSION(f) (!!(m_config.version == ArchXT_ADC2Config::V ## f))


ArchXT_ADC2::ArchXT_ADC2(int num, const ArchXT_ADC2Config& config)
:Peripheral(AVR_IOCTL_ADC(0x30 + num))
,m_config(config)
,m_state(ADC_Disabled)
,m_init(0)
,m_timer(*this, &ArchXT_ADC2::timer_raised)
,m_accum_counter(0)
,m_sample(0)
,m_result(0)
,m_sample_unread(false)
,m_result_unread(false)
,m_smp_intflag(false)
,m_res_intflag(false)
,m_err_intflag(false)
{}


bool ArchXT_ADC2::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), ADC_RUNSTDBY_bm | ADC_LOWLAT_bm | ADC_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), ADC_PRESC_gm);
    add_ioreg(REG_ADDR(CTRLC), ADC_TIMEBASE_gm | ADC_REFSEL_gm);
    add_ioreg(REG_ADDR(CTRLD), ADC_WINSRC_bm | ADC_WINCM_gm);
    add_ioreg(REG_ADDR(INTCTRL), ADC_TRIGOVR_bm | ADC_SAMPOVR_bm | ADC_RESOVR_bm |
                                 ADC_WCMP_bm | ADC_SAMPRDY_bm | ADC_RESRDY_bm);
    add_ioreg(REG_ADDR(INTFLAGS), ADC_TRIGOVR_bm | ADC_SAMPOVR_bm | ADC_RESOVR_bm |
                                 ADC_WCMP_bm | ADC_SAMPRDY_bm | ADC_RESRDY_bm, IORegister::Strobe);
    add_ioreg(REG_ADDR(STATUS), ADC_ADCBUSY_bm, IORegister::RO);
    //DBGCTRL not implemented
    add_ioreg(REG_ADDR(CTRLE), ADC_SAMPDUR_gm);
    add_ioreg(REG_ADDR(CTRLF), ADC_FREERUN_bm | ADC_LEFTADJ_bm | ADC_SAMPNUM_gm);
    add_ioreg(REG_ADDR(COMMAND), ADC_MODE_gm | ADC_START_gm);
    add_ioreg(REG_ADDR(MUXPOS), ADC_MUXPOS_gm);

    if (TEST_VERSION(1)) { //If has PGA
        add_ioreg(REG_ADDR(PGACTRL), ADC_GAIN_gm | ADC_PGABIASSEL_gm | ADC_ADCPGASAMPDUR_gm | ADC_PGAEN_bm);
        add_ioreg(REG_ADDR(MUXPOS), ADC_VIA_gm);
    }

    if (TEST_VERSION(1)) { //If has differential
        add_ioreg(REG_ADDR(COMMAND), ADC_DIFF_bm);
        add_ioreg(REG_ADDR(MUXNEG), ADC_MUXNEG_gm);
        if (TEST_VERSION(1)) { //If has PGA
            add_ioreg(REG_ADDR(MUXNEG), ADC_VIA_gm);
        }
    }

    add_ioreg(REG_ADDR(RESULT0), IORegister::RO);
    add_ioreg(REG_ADDR(RESULT1), IORegister::RO);
    if (TEST_VERSION(1)) { //If has 12-bits resolution
        add_ioreg(REG_ADDR(RESULT2), IORegister::RO);
        add_ioreg(REG_ADDR(RESULT3), IORegister::RO);
    }
    add_ioreg(REG_ADDR(SAMPLEL), IORegister::RO);
    add_ioreg(REG_ADDR(SAMPLEH), IORegister::RO);
    //TEMP0/1/2 not implemented
    add_ioreg(REG_ADDR(WINLTL));
    add_ioreg(REG_ADDR(WINLTH));
    add_ioreg(REG_ADDR(WINHTL));
    add_ioreg(REG_ADDR(WINHTH));

    status &= m_err_intflag.init(device,
                                 REG_ADDR(INTCTRL), REG_ADDR(INTFLAGS),
                                 ADC_TRIGOVR_bm | ADC_SAMPOVR_bm | ADC_RESOVR_bm,
                                 m_config.iv_error);

    status &= m_smp_intflag.init(device,
                                 REG_ADDR(INTCTRL), REG_ADDR(INTFLAGS),
                                 ADC_WCMP_bm | ADC_SAMPRDY_bm,
                                  m_config.iv_sampready);

    status &= m_res_intflag.init(device,
                                 REG_ADDR(INTCTRL), REG_ADDR(INTFLAGS),
                                 ADC_WCMP_bm | ADC_RESRDY_bm,
                                 m_config.iv_resready);

    m_timer.init(*device.cycle_manager());

    return status;
}


void ArchXT_ADC2::reset(int)
{
    m_state = ADC_Disabled;
    m_init = 0;
    m_accum_counter = 0;
    m_sample = 0;
    m_result = 0;
    m_sample_unread = false;
    m_result_unread = false;
    m_timer.cancel();

    //Reset the internal reference
    auto cfg = find_reg_config_p(m_config.references, 0x00);
    VREF::getset_t vref_reqinfo = { cfg->source, m_config.vref_channel, cfg->level };
    ctlreq_data_t reqdata = { .data = &vref_reqinfo };
    device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_SET_MUX, &reqdata);
}


bool ArchXT_ADC2::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_ADC_TRIGGER) {
        if (m_state == ADC_Idle && READ_IOREG_F_GC(COMMAND, ADC_START) == ADC_START_EVENT_TRIGGER_gc)
            start_conversion_cycle();
        return true;
    }
    return false;
}


uint8_t ArchXT_ADC2::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    //Reading the sample clears the SAMPRDY interrupt flag
    if (addr == REG_ADDR(SAMPLEL)) {
        m_sample_unread = false;
        m_smp_intflag.clear_flag(ADC_SAMPRDY_bm);
    }

    //Reading the result clears the RESRDY interrupt flag
    else if (addr == REG_ADDR(RESULT0)) {
        m_result_unread = false;
        m_res_intflag.clear_flag(ADC_RESRDY_bm);
    }

    return value;
}


uint8_t ArchXT_ADC2::ioreg_peek_handler(reg_addr_t addr, uint8_t value)
{
    return value;
}


void ArchXT_ADC2::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        //Positive edge on the enable bit (CTRLA.ENABLE).
        //We reset the state and the prescaler
        if (data.posedge() & ADC_ENABLE_bm) {
            VREF::getset_t reqinfo = { .source = VREF::Source_VCC };
            ctlreq_data_t reqdata = { .data = &reqinfo };
            if (device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata)) {
                m_state = ADC_Idle;
                m_accum_counter = 0;
                m_init = INIT_FLAG_ALL;
            }
        }
        //Negative edge on the enable bit (CTRLA.ENABLE).
        //We disable the ADC, stop the cycle timer (in case a conversion is running)
        else if (data.negedge() & ADC_ENABLE_bm) {
            m_timer.cancel();
            m_state = ADC_Disabled;
        }
    }

    else if (reg_ofs == REG_OFS(CTRLC)) {
        uint8_t ref_sel_value = EXTRACT_F(data.value, ADC_REFSEL);
        auto cfg = find_reg_config_p(m_config.references, ref_sel_value);
        if (cfg) {
            VREF::getset_t vref_reqinfo = { cfg->source, m_config.vref_channel, cfg->level };
            ctlreq_data_t reqdata = { .data = &vref_reqinfo };
            if (!device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_SET_MUX, &reqdata))
                _CRASH("ADC: Unable to change the voltage reference");
        }

        //Indicates that the reference module needs reinitialisation time
        m_init |= INIT_FLAG_REF;
    }

    else if (reg_ofs == REG_OFS(COMMAND)) {
        auto conv_cfg = find_reg_config_p(m_config.modes, EXTRACT_F(data.value, ADC_MODE));
        if (conv_cfg)
            m_conv_mode = *conv_cfg;
        else
            m_conv_mode = ArchXT_ADC2Config::conversion_config_t();

        //Switching to a single mode resets the accumulator
        if (!m_conv_mode.accum)
            m_accum_counter = 0;

        //Writing IMMEDIATE to START when it's idle starts a conversion cycle
        if (m_state == ADC_Idle && EXTRACT_GC(data.value, ADC_START) == ADC_START_IMMEDIATE_gc) {
            logger().dbg("Conversion started from command");
            start_conversion_cycle();
        }
        //Writing STOP to START when it's converting stops the conversion cycle
        else if (m_state > ADC_Idle && EXTRACT_GC(data.value, ADC_START) == ADC_START_STOP_gc) {
            logger().dbg("Conversion cancelled from command");
            m_timer.cancel();
            m_state = ADC_Idle;
            m_accum_counter = 0;
        }
    }

    else if (reg_ofs == REG_OFS(PGACTRL)) {
        //Indicates that the PGA needs initialisation delay
        m_init |= INIT_FLAG_PGA;
    }

    else if (reg_ofs == REG_OFS(MUXPOS)) {
        //If the mux selection changes, the ADC needs initialisation delay
        if (data.anyedge())
            m_init |= INIT_FLAG_ADC;
        //If the trigger is set to MUXPOS, start a conversion
        if (READ_IOREG_F_GC(COMMAND, ADC_START) == ADC_START_MUXPOS_WRITE_gc)
            start_conversion_cycle();
    }

    else if (reg_ofs == REG_OFS(MUXNEG)) {
        //If the mux selection changes, the ADC needs initialisation delay
        if (data.anyedge())
            m_init |= INIT_FLAG_ADC;
        //If the trigger is set to MUXNEG, start a conversion
        if (READ_IOREG_F_GC(COMMAND, ADC_START) == ADC_START_MUXNEG_WRITE_gc)
            start_conversion_cycle();
    }
}


void ArchXT_ADC2::start_conversion_cycle()
{
    m_state = ADC_Initialisation;

    unsigned long ps_factor = ClockPrescalerFactors[READ_IOREG_F(CTRLB, ADC_PRESC)];

    SET_IOREG(STATUS, ADC_ADCBUSY);

    if (m_init) {

        //Calculate the initialisation delay in us
        double ref_period_us = 1e6 / device()->cycle_manager()->reference_frequency();
        double init_delay_us = 0;

        if (m_init & INIT_FLAG_ADC) {
            init_delay_us = std::max(ps_factor * INIT_DELAY_ADC_CYCLES * ref_period_us, INIT_DELAY_ADC_MIN);

            auto pos_ch_config = find_reg_config_p(m_config.pos_channels, READ_IOREG_F(MUXPOS, ADC_MUXPOS));
            if (pos_ch_config) {
                if (pos_ch_config->type == Channel_Temperature)
                    init_delay_us = std::max(init_delay_us, INIT_DELAY_TEMP);
                else if (pos_ch_config->type == Channel_AcompRef)
                    init_delay_us = std::max(init_delay_us, INIT_DELAY_DAC);
            }
        }

        //PGA module initialisation delay
        if ((m_init & INIT_FLAG_PGA) && TEST_IOREG(PGACTRL, ADC_PGAEN))
            init_delay_us = std::max(init_delay_us, INIT_DELAY_PGA);

        //Reference module initialisation delay
        if (m_init & INIT_FLAG_REF) {
            auto ref_config = find_reg_config_p(m_config.references, READ_IOREG_F(CTRLC, ADC_REFSEL));
            if (ref_config && ref_config->source == VREF::Source_Bandgap)
                init_delay_us = std::max(init_delay_us, INIT_DELAY_REF);
        }

        m_timer.delay_s(init_delay_us / 1e6);

    } else {

        m_timer.delay(ps_factor);

    }

    if (!m_accum_counter) {
        m_result = 0;
        if (m_conv_mode.accum)
            m_accum_counter = 1 << READ_IOREG_F(CTRLF, ADC_SAMPNUM);
        else
            m_accum_counter = 1;
    }

    //Raise the signal
    m_signal.raise(Signal_ConversionStarted);
}

/*
* Main function for reading and converting analog values
*/

#define _CRASH_CHANNEL(text) \
    do { \
        device()->crash(CRASH_BAD_CTL_IO, text); \
        return { false, 0.0 }; \
    } while(0);

std::tuple<bool, double> ArchXT_ADC2::read_channel(const channel_config_t& ch_config)
{
    //Obtain the raw analog value depending on the channel mux configuration
    //The raw value is in the interval [0.0; 1.0]
    //and is relative to VCC
    double channel_value;
    switch(ch_config.type) {

    case Channel_SingleEnded: {
            Pin* p = device()->find_pin(ch_config.pin_p);
            if (!p) _CRASH_CHANNEL("Invalid pin configuration");
            m_signal.raise(Signal_AboutToSamplePin, ch_config.pin_p);
            channel_value = p->voltage();
        } break;

    case Channel_IntRef: {
        VREF::getset_t reqinfo = { .source = VREF::Source_Mux, .channel = m_config.vref_channel };
        ctlreq_data_t reqdata = { .data = &reqinfo };
        device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata);
        channel_value = reqinfo.voltage;
    } break;

    case Channel_AcompRef: {
        ctlreq_data_t reqdata;
        if (!device()->ctlreq(AVR_IOCTL_ACP(ch_config.per_num), AVR_CTLREQ_ACP_GET_DAC, &reqdata))
            _CRASH_CHANNEL("Unable to obtain the DAC reference from the Analog Comparator");
        channel_value = reqdata.data.as_double();
    } break;

    case Channel_VDDDiv10: {
        VREF::getset_t reqinfo = { .source = VREF::Source_VCC };
        ctlreq_data_t reqdata = { .data = &reqinfo };
        device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata);
        channel_value = 0.1 * reqinfo.voltage;
    } break;

    case Channel_Temperature: {
        m_signal.raise(Signal_AboutToSampleTemp);
        VREF::getset_t reqinfo = { .source = VREF::Source_Temperature };
        ctlreq_data_t reqdata = { .data = &reqinfo };
        device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata);
        channel_value = reqinfo.voltage;
    } break;

    case Channel_Zero:
        channel_value = 0.0; break;

    default:
        _CRASH_CHANNEL("Channel type not supported");break;
    }

    if (channel_value < 0.0) channel_value = 0.0;
    if (channel_value > 1.0) channel_value = 1.0;

    return { true, channel_value };
}


void ArchXT_ADC2::convert()
{
    //Read register settings used throughout the conversion process
    bool diff_mode = TEST_IOREG(COMMAND, ADC_DIFF);

    logger().dbg("Reading analog value");

    auto pos_ch_config = find_reg_config_p(m_config.pos_channels, READ_IOREG_F(MUXPOS, ADC_MUXPOS));
    if (!pos_ch_config)
        _CRASH("ADC: Invalid channel configuration");
    auto [ok, pos_value] = read_channel(*pos_ch_config);
    if (!ok) return;

    double neg_value;
    if (diff_mode) {
        auto neg_ch_config = find_reg_config_p(m_config.neg_channels, READ_IOREG_F(MUXNEG, ADC_MUXNEG));
        if (!neg_ch_config)
            _CRASH("ADC: Invalid channel configuration");
        auto [ok, neg_value] = read_channel(*neg_ch_config);
        if (!ok) return;
    } else {
        neg_value = 0.0;
    }

    double raw_value = pos_value - neg_value;

    //Apply the PGA gain
    if (READ_IOREG_F(MUXPOS, ADC_VIA) && TEST_IOREG(PGACTRL, ADC_PGAEN)) {
        auto pga_gain_config = find_reg_config_p(PGAGains, READ_IOREG_F(PGACTRL, ADC_GAIN));
        if (!pga_gain_config)
            _CRASH("ADC: Invalid channel configuration");
        raw_value *= pga_gain_config->gain;
    }

    //Find the reference voltage mux configuration and request the value from the VREF peripheral
    double vref = 0.0;
    auto ref_config = find_reg_config_p(m_config.references, READ_IOREG_F(CTRLC, ADC_REFSEL));
    if (!ref_config)
        _CRASH("ADC: Invalid reference configuration");
    VREF::getset_t reqinfo = { .source = VREF::Source_Mux, .channel = m_config.vref_channel };
    ctlreq_data_t reqdata = { .data = &reqinfo };
    device()->ctlreq(AVR_IOCTL_VREF, AVR_CTLREQ_VREF_GET, &reqdata);
    vref = reqinfo.voltage;
    if (vref == 0.0)
        _CRASH("ADC: Zero voltage reference");

    //Make the final value relative to VREF
    raw_value /= vref;

    //Constraint the raw analog value to the interval [-1.0; +1.0]
    if (raw_value < -1.0) raw_value = -1.0;
    if (raw_value > 1.0) raw_value = 1.0;

    //Determine the integer range depending on the 8/10/12 bits and diff settings
    int int_range;
    if (!m_conv_mode.extbits) {
        int_range = diff_mode ? 128 : 256;
    }
    else if (TEST_VERSION(1)) {
        int_range = diff_mode ? 2048 : 4096;
    }
    else {
        int_range = diff_mode ? 512 : 1024;
    }

    int int_min = diff_mode ? -int_range : 0;

    //Convert the signed/unsigned 8/10/12 bits integer
    m_sample = lrint(raw_value * int_range);
    if (m_sample > (int_range - 1)) m_sample = int_range - 1;
    if (m_sample < int_min) m_sample = int_min;

    logger().dbg("Sampled raw=%f => result=%hd", raw_value, m_sample);

    //Accumulator
    m_result += m_sample;
    --m_accum_counter;

    logger().dbg("Accumulator (int): %ld (counter=%hu)", m_result, m_accum_counter);
}


bool ArchXT_ADC2::check_window(unsigned int value)
{
    uint8_t win_mode = READ_IOREG_F(CTRLE, ADC_WINCM);
    if (win_mode == ADC_WINCM_NONE_gc)
        return false;

    unsigned short winlo = ((uint16_t) READ_IOREG(WINLTH) << 8) |
                            (uint16_t) READ_IOREG(WINLTL);
    unsigned short winhi = ((uint16_t) READ_IOREG(WINHTH) << 8) |
                            (uint16_t) READ_IOREG(WINHTL);

    bool win_int;
    switch(win_mode) {
        case ADC_WINCM_BELOW_gc:
            win_int = (m_result < winlo);
            break;
        case ADC_WINCM_ABOVE_gc:
            win_int = (m_result > winhi);
            break;
        case ADC_WINCM_INSIDE_gc:
            win_int = (m_result > winlo && m_result < winhi);
            break;
        case ADC_WINCM_OUTSIDE_gc:
            win_int = (m_result < winlo || m_result > winhi);
            break;
        default:
            win_int = false;
            break;
    }

    if (win_int)
        logger().dbg("Triggering WINCOMP interrupt");

    return win_int;
}


void ArchXT_ADC2::format_sample()
{
    //Format the sample value for storage
    uint16_t reg_sample;
    if (TEST_IOREG(COMMAND, ADC_DIFF))
        reg_sample = (int16_t) m_sample;
    else
        reg_sample = (uint16_t) m_sample;

    if (TEST_IOREG(CTRLF, ADC_LEFTADJ) && m_conv_mode.extbits)
        reg_sample <<= 4;

    WRITE_IOREG(SAMPLEL, reg_sample & 0x00FF);
    WRITE_IOREG(SAMPLEH, (reg_sample >> 8) & 0xFF);

    if (m_sample_unread)
        m_err_intflag.set_flag(ADC_SAMPOVR_bm);
    m_sample_unread = true;

    uint8_t int_flag = ADC_SAMPRDY_bm;
    if ((READ_IOREG(CTRLD) & ADC_WINSRC_bm) == ADC_WINSRC_SAMPLE_gc && check_window(reg_sample))
        int_flag |= ADC_WCMP_bm;
    m_smp_intflag.set_flag(int_flag);
}


void ArchXT_ADC2::format_result()
{
    uint8_t fmt_reg = READ_IOREG(CTRLF);
    bool left_adj = EXTRACT_B(fmt_reg, ADC_LEFTADJ);

    uint8_t sampnum = m_conv_mode.accum ? EXTRACT_F(fmt_reg, ADC_SAMPNUM) : 0;

    //Format the result value for storage
    uint32_t reg_result;
    if (TEST_IOREG(COMMAND, ADC_DIFF))
        reg_result = (int32_t) m_result;
    else
        reg_result = (uint32_t) m_result;

    if (m_conv_mode.scaling && sampnum > 4)
        reg_result >>= sampnum - 4;

    if (left_adj && (m_conv_mode.extbits && (!m_conv_mode.accum || (m_conv_mode.scaling && sampnum < 4))))
        reg_result <<= 4 - sampnum;

    WRITE_IOREG(RESULT0, reg_result & 0x00FF);
    WRITE_IOREG(RESULT1, (reg_result >> 8) & 0xFF);
    if (TEST_VERSION(1)) {
        WRITE_IOREG(RESULT2, (reg_result >> 16) & 0xFF);
        WRITE_IOREG(RESULT3, (reg_result >> 24) & 0xFF);
    }

    //Set the unread/overrun flags
    if (m_result_unread)
        m_err_intflag.set_flag(ADC_RESOVR_bm);
    m_result_unread = true;

    //Set the ready flag.
    //If the result is set as the source for the window check, run the window logic.
    uint8_t int_flag = ADC_RESRDY_bm;
    if ((READ_IOREG(CTRLD) & ADC_WINSRC_bm) == ADC_WINSRC_RESULT_gc && check_window(reg_result))
        int_flag |= ADC_WCMP_bm;
    m_res_intflag.set_flag(int_flag);
}


unsigned long ArchXT_ADC2::sampling_delay() const
{
    //Calculate the delay in half ADC cycles for sampling and conversion
    unsigned long d = 1 + 2 * READ_IOREG_F(CTRLE, ADC_SAMPDUR);

    //Add the PGA sample delay if enabled
    uint8_t reg_pga = READ_IOREG(PGACTRL);
    if (reg_pga & ADC_PGAEN_bm)
        d += 1 + PGASampleDelays[EXTRACT_F(reg_pga, ADC_ADCPGASAMPDUR)];

    //Add the conversion delay: 11 ADC cycles for 8 bits or 15 for 10/12 bits
    d += m_conv_mode.extbits ? 30 : 22;

    return d;
}


/*
 * Callback from the timer hook.
 * We arrive here 3 times in a conversion cycle.
 * First, we perform the actual analog read.
 * Second, we store it in the data register and raise the interrupt flag
 */
void ArchXT_ADC2::timer_raised()
{
    unsigned long ps_factor = ClockPrescalerFactors[READ_IOREG_F(CTRLB, ADC_PRESC)];

    if (m_state == ADC_Initialisation) {
        m_state = ADC_Sampling;
        m_timer.delay(ps_factor);
    }

    else if (m_state == ADC_Sampling) {
        convert();
        m_state = ADC_Converting;
        m_timer.delay(ps_factor * sampling_delay());
    }

    else if (m_state == ADC_Converting) {
        logger().dbg("Conversion complete");

        format_sample();

        m_signal.raise(Signal_ConversionComplete);

        //At the end of an accumulation, write the result
        if (!m_accum_counter)
            format_result();

        //If a burst is in progress, restart a conversion in one half ADC clock cycle
        if (m_accum_counter && m_conv_mode.burst) {
            m_state = ADC_Sampling;
            m_timer.delay(ps_factor);
            m_signal.raise(Signal_ConversionStarted);
        }
        //If free run mode is enabled, start immediately another conversion/accumulation cycle
        else if (TEST_IOREG(CTRLA, ADC_FREERUN)) {
            logger().dbg("Conversion started from free run");
            start_conversion_cycle();
        }
        //End of conversion, return to idle state
        else {
            CLEAR_IOREG(STATUS, ADC_ADCBUSY);
            WRITE_IOREG_F_GC(COMMAND, ADC_START, ADC_START_STOP_gc);
            m_state = ADC_Idle;
        }
    }
}

/*
 * The ADC is paused for modes above Standby and in Standby if RUNSTBY is not set
 */
void ArchXT_ADC2::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !TEST_IOREG(CTRLA, ADC_RUNSTDBY))) {
        if (on){
            logger().dbg("Pausing");
            m_timer.pause();
        } else {
            logger().dbg("Resuming");
            m_timer.resume();
        }
    }
}
