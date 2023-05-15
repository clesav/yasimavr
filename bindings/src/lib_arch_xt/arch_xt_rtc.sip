/*
 * arch_xt_rtc.sip
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

struct ArchXT_RTCConfig {
%TypeHeaderCode
#include "arch_xt_rtc.h"
%End

    enum RTC_ClockSource {
        Clock_32kHz,
        Clock_1kHz
    };

    struct clksel_config_t : base_reg_config_t {
        RTC_ClockSource source;
    };

    std::vector<ArchXT_RTCConfig::clksel_config_t> clocks;
    reg_addr_t reg_base;
    int_vect_t iv_rtc;
    int_vect_t iv_pit;

};


class ArchXT_RTC : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_rtc.h"
%End

public:

    ArchXT_RTC(const ArchXT_RTCConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, SleepMode);

};
