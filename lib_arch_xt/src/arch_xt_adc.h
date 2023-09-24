/*
 * arch_xt_adc.h
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


#ifndef __YASIMAVR_XT_ADC_H__
#define __YASIMAVR_XT_ADC_H__

#include "arch_xt_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_adc.h"
#include "ioctrl_common/sim_timer.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_adc
   \brief Configuration structure for ArchXT_ADC.
 */
struct ArchXT_ADCConfig {

    struct reference_config_t : base_reg_config_t {
        VREF::Source source;
    };

    /// List of the ADC channels
    std::vector<ADC::channel_config_t> channels;
    /// List of the voltage references
    std::vector<reference_config_t> references;
    /// Channel index for the voltage reference
    unsigned int vref_channel;
    /// List of the clock prescaler factors
    std::vector<unsigned long> clk_ps_factors;
    /// Wrapping value for the ADC clock prescaler
    unsigned long clk_ps_max;
    /// List of conversion delay values
    std::vector<unsigned long> init_delays;
    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for ADC_RESREADY
    int_vect_t iv_resready;
    /// Interrupt vector index for ADC_WINCMP
    int_vect_t iv_wincmp;
    /// Temperature sensor calibration offset (in V at +25°C)
    double temp_cal_25C;
    /// Temperature sensor calibration linear coef (in V/°C)
    double temp_cal_coef;

};

/**
   \ingroup api_adc
   \brief Implementation of an ADC for XT series

   Limitations:
    - Sampling cap and duty cycle settings have no effect
    - No Debug Run override

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL : returns a pointer to the instance signal
    - AVR_CTLREQ_ADC_SET_TEMP : Sets the temperature reported by the internal sensor.
    The reqdata should carry the temperature in Celsius as a double.
    - AVR_CTLREQ_ADC_TRIGGER : Allows other peripherals to trigger a conversion.
    The trigger only works when the ADC is enabled and idle, and the bit STARTEI is set.
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_ADC : public ADC,
                                         public Peripheral,
                                         public SignalHook {

public:

    ArchXT_ADC(int num, const ArchXT_ADCConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    enum State {
        ADC_Disabled,
        ADC_Idle,
        ADC_Starting,
        ADC_PendingConversion,
        ADC_PendingRaise,
    };

    const ArchXT_ADCConfig& m_config;
    State m_state;
    bool m_first;
    PrescaledTimer m_timer;
    double m_temperature;
    uint8_t m_latched_ch_mux;
    uint8_t m_latched_ref_mux;
    uint8_t m_accum_counter;
    uint16_t m_result;
    uint16_t m_win_lothres;
    uint16_t m_win_hithres;
    InterruptFlag m_res_intflag;
    InterruptFlag m_cmp_intflag;
    Signal m_signal;

    void start_conversion_cycle();
    void read_analog_value();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_ADC_H__
