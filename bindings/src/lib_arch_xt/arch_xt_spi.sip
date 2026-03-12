/*
 * arch_xt_spi.sip
 *
 *  Copyright 2022-2026 Clement Savergne <csavergne@yahoo.com>

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

struct ArchXT_SPIConfig {
%TypeHeaderCode
#include "arch_xt_spi.h"
%End

    reg_addr_t reg_base;
    int_vect_t iv_spi;

};


class ArchXT_SPI : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_spi.h"
%End

public:

    ArchXT_SPI(int, const ArchXT_SPIConfig& /KeepReference/);

};
