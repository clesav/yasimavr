/*
 * yasimavr-core.sip
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

%Module(name=yasimavr.lib.core)

%License "GPLv3 (see LICENSE file)"

%DefaultEncoding(name = "UTF-8")

typedef unsigned long long      uint64_t;
typedef long long               int64_t;
typedef unsigned int            uint32_t;
typedef int                     int32_t;
typedef unsigned short          uint16_t;
typedef short                   int16_t;
typedef unsigned char           uint8_t /PyInt/;

%UnitPostIncludeCode
#include "core/sim_globals.h"
YASIMAVR_USING_NAMESPACE
%End

%Include support/stdvector.sip
%Include support/stdstring.sip

//=======================================================================================

%Include core/config.sip
%Include core/core.sip
%Include core/cycle_timer.sip
%Include core/debug.sip
%Include core/device.sip
%Include core/firmware.sip
%Include core/logger.sip
%Include core/interrupt.sip
%Include core/memory.sip
%Include core/peripheral.sip
%Include core/ioreg.sip
%Include core/pin.sip
%Include core/signal.sip
%Include core/sleep.sip
%Include core/types.sip

//=======================================================================================

%Include ioctrl_common/acp.sip
%Include ioctrl_common/adc.sip
%Include ioctrl_common/port.sip
%Include ioctrl_common/spi.sip
%Include ioctrl_common/timer.sip
%Include ioctrl_common/twi.sip
%Include ioctrl_common/uart.sip
%Include ioctrl_common/vref.sip
%Include ioctrl_common/wdt.sip

//=======================================================================================

%Include sim/sim_loop.sip
