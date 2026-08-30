/*
 * arch_xt_adc2.sip
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

struct ArchXT_ADC2Config {
%TypeHeaderCode
#include "arch_xt_adc2.h"
%End

    enum Version {
        V1,
        V2,
    };

    struct conversion_config_t : base_reg_config_t {
        uint8_t mode;
    };

    std::vector<ArchXT_ADC2Config::conversion_config_t> modes;
    std::vector<ADC::channel_config_t> pos_channels;
    std::vector<ADC::channel_config_t> neg_channels;
    std::vector<ADC::reference_config_t> references;
    unsigned int vref_channel;
    UnsignedLongVector clk_ps_factors;
    unsigned long clk_ps_max;

    reg_addr_t reg_base;

    int_vect_t iv_error;
    int_vect_t iv_sampready;
    int_vect_t iv_resready;

    ArchXT_ADC2Config::Version version;

};


class ArchXT_ADC2 : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_adc2.h"
%End

public:

    ArchXT_ADC2(int, const ArchXT_ADC2Config& /KeepReference/);

};
