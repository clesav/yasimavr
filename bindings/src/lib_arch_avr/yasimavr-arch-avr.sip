/*
 * yasimavr-arch-avr.sip
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

%Module(name=yasimavr.lib.arch_avr)

%Import ../lib_core/yasimavr-core.sip

%License "GPLv3 (see LICENSE file)"

%DefaultEncoding(name = "UTF-8")

%UnitPostIncludeCode
#include "core/sim_globals.h"
YASIMAVR_USING_NAMESPACE
%End

//=======================================================================================

%Include arch_avr_acp.sip
%Include arch_avr_adc.sip
%Include arch_avr_device.sip
%Include arch_avr_extint.sip
%Include arch_avr_misc.sip
%Include arch_avr_port.sip
%Include arch_avr_spi.sip
%Include arch_avr_timer.sip
%Include arch_avr_twi.sip
%Include arch_avr_usart.sip
%Include arch_avr_wdt.sip
