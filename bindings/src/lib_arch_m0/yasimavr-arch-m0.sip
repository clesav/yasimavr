/*
 * yasimavr-arch-m0.sip
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

%Module(name=yasimavr.lib.arch_m0)

%Import ../lib_core/yasimavr-core.sip

%License "GPLv3 (see LICENSE file)"

%DefaultEncoding(name = "UTF-8")

//=======================================================================================

%Include arch_mega0_adc.sip
%Include arch_mega0_device.sip
%Include arch_mega0_misc.sip
%Include arch_mega0_nvm.sip
%Include arch_mega0_port.sip
%Include arch_mega0_rtc.sip
%Include arch_mega0_spi.sip
%Include arch_mega0_timer_a.sip
%Include arch_mega0_timer_b.sip
%Include arch_mega0_twi.sip
%Include arch_mega0_usart.sip
%Include arch_mega0_wdt.sip
