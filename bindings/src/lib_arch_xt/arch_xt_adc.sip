/*
 * arch_xt_adc.sip
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

struct ArchXT_ADC_Config {
%TypeHeaderCode
#include "arch_xt_adc.h"
%End

    struct reference_config_t : base_reg_config_t {
        IO_VREF::Source source;
    };

    std::vector<IO_ADC::channel_config_t> channels;
    std::vector<ArchXT_ADC_Config::reference_config_t> references;
    uint32_t vref_channel;
    UnsignedInt16Vector clk_ps_factors;
    uint16_t clk_ps_max;
    UnsignedInt16Vector init_delays;

    reg_addr_t reg_base;

    int_vect_t iv_resready;
    int_vect_t iv_wincmp;

    double temp_cal_25C;
    double temp_cal_coef;

};


class ArchXT_ADC : public IO_ADC, public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_adc.h"
%End

public:

    ArchXT_ADC(int, const ArchXT_ADC_Config& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, SleepMode);
    virtual void raised(const signal_data_t&, uint16_t);

};
