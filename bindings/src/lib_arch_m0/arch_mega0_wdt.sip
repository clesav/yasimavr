/*
 * arch_mega0_wdt.sip
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

struct AVR_ArchMega0_WDT_Config {
%TypeHeaderCode
#include "arch_mega0_wdt.h"
%End

    uint32_t clock_frequency;
    UnsignedInt32Vector delays;

    reg_addr_t reg_base;

};

class AVR_ArchMega0_WDT : public AVR_WatchdogTimer /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_wdt.h"
%End

public:

    AVR_ArchMega0_WDT(const AVR_ArchMega0_WDT_Config&);

    virtual bool init(AVR_Device&);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

protected:

    virtual void timeout();

};
