/*
 * arch_xt_clock.sip
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

class ArchXT_ClkCtrl : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_clock.h"
%End

public:

    static const sim_id_t ClkSrc_Main;
    static const sim_id_t ClkSrc_ULP32K;
    static const sim_id_t ClkSrc_EXT32K;
    static const sim_id_t ClkSrc_EXTCLK;

    ArchXT_ClkCtrl(reg_addr_t);

};
