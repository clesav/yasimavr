/*
 * arch_xt_adc2.h
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

#ifndef __YASIMAVR_XT_ADC2_H__
#define __YASIMAVR_XT_ADC2_H__

#include "arch_xt_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_timer.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_adc
   \brief Configuration structure for ArchXT_ADC2.
 */
struct ArchXT_ADC2Config {

    enum Version {
        /// Version with differential mode, PGA and 12-bits resolution
        V1,
        /// No differential, no PGA, 10-bits resolution
        V2
    };

    struct conversion_config_t : base_reg_config_t {
        union {
            uint8_t mode;
            struct {
                uint8_t extbits : 1;
                uint8_t accum : 1;
                uint8_t burst : 1;
                uint8_t scaling : 1;
            };
        };
    };

    /// List of the conversion modes
    std::vector<conversion_config_t> modes;
    /// List of the ADC channels (positive polarity)
    std::vector<ADC::channel_config_t> pos_channels;
    /// List of the ADC channels (negative polarity)
    std::vector<ADC::channel_config_t> neg_channels;
    /// List of the voltage references
    std::vector<ADC::reference_config_t> references;
    /// Channel index for the voltage reference
    unsigned int vref_channel;
    /// List of the clock prescaler factors
    std::vector<unsigned long> clk_ps_factors;
    /// Wrapping value for the ADC clock prescaler
    unsigned long clk_ps_max;
    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for ADC_ERROR
    int_vect_t iv_error;
    /// Interrupt vector index for ADC_SAMPREADY
    int_vect_t iv_sampready;
    /// Interrupt vector index for ADC_WINCMP
    int_vect_t iv_resready;
    /// Temperature sensor calibration offset (in V at +25°C)
    double temp_cal_25C;
    /// Temperature sensor calibration linear coef (in V/°C)
    double temp_cal_coef;
    /// Model version
    Version version;

};

/**
   \ingroup api_adc
   \brief Implementation of an ADC model 2 for XT series

   Limitations: TBC

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL : returns a pointer to the instance signal
    - AVR_CTLREQ_ADC_TRIGGER : Allows other peripherals to trigger a conversion.
    The trigger only works when the ADC is enabled and idle, and START is set to EVENNT TRIGGER.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_ADC2 : public Peripheral {

public:

    ArchXT_ADC2(int num, const ArchXT_ADC2Config& config);

    virtual bool init(Device& device) override;
    virtual void reset(int flags) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;

private:

    enum State {
        ADC_Disabled,
        ADC_Idle,
        ADC_Initialisation,
        ADC_Sampling,
        ADC_Converting,
    };

    const ArchXT_ADC2Config& m_config;
    State m_state;
    uint8_t m_init;
    BoundFunctionCycleTimer<ArchXT_ADC2> m_timer;
    ArchXT_ADC2Config::conversion_config_t m_conv_mode;
    unsigned int m_accum_counter;
    int m_sample;
    int m_result;
    bool m_sample_unread;
    bool m_result_unread;
    InterruptFlag m_smp_intflag;
    InterruptFlag m_res_intflag;
    InterruptFlag m_err_intflag;
    Signal m_signal;

    void start_conversion_cycle();
    std::tuple<bool, double> read_channel(const ADC::channel_config_t& config);
    void convert();
    bool check_window(unsigned int value);
    void format_sample();
    void format_result();
    unsigned long sampling_delay() const;
    void timer_raised();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_ADC2_H__
