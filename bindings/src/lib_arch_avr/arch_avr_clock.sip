/*
 * arch_avr_clock.sip
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

struct ArchAVR_ClkCtrlConfig {
%TypeHeaderCode
#include "arch_avr_clock.h"
%End

    struct clk_config_t {
        sim_id_t id;
        bool ext;
        double freq;
        uint8_t sel_lo;
        uint8_t sel_hi;
    };

    std::vector<ArchAVR_ClkCtrlConfig::clk_config_t> clk_configs;

    regbit_t rb_clkps;

};


class ArchAVR_ClkCtrl : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_clock.h"
%End

public:

    ArchAVR_ClkCtrl(const ArchAVR_ClkCtrlConfig& /KeepReference/);

};
