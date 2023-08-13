/*
 * arch_xt_wdt.sip
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

struct ArchXT_WDTConfig {
%TypeHeaderCode
#include "arch_xt_wdt.h"
%End

    unsigned long clock_frequency;
    UnsignedLongVector delays;

    reg_addr_t reg_base;

};


class ArchXT_WDT : public WatchdogTimer /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_wdt.h"
%End

public:

    ArchXT_WDT(const ArchXT_WDTConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

protected:

    virtual void timeout();

};
