/*
 * arch_avr_wdt.sip
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

struct ArchAVR_WDTConfig {
%TypeHeaderCode
#include "arch_avr_wdt.h"
%End

    unsigned long clock_frequency;
    UnsignedLongVector delays;

    reg_addr_t        reg_wdt;
    regbit_compound_t rbc_delay;
    bitspec_t         bs_chg_enable;
    bitspec_t         bs_reset_enable;
    bitspec_t         bs_int_enable;
    bitspec_t         bs_int_flag;
    regbit_t          rb_reset_flag;

    int_vect_t        iv_wdt;

};

class ArchAVR_WDT : public Peripheral, public InterruptHandler /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_wdt.h"
%End

public:

    ArchAVR_WDT(const ArchAVR_WDTConfig& /KeepReference/);

};
