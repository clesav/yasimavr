/*
 * arch_xt_wdt.h
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

#ifndef __YASIMAVR_XT_WDT_H__
#define __YASIMAVR_XT_WDT_H__

#include "ioctrl_common/sim_wdt.h"


//=======================================================================================
/*
 * Implementation of a Watchdog Timer for XT core series
 */

struct AVR_ArchXT_WDT_Config {

    uint32_t clock_frequency;
    std::vector<uint32_t> delays;

    reg_addr_t reg_base;

};


class DLL_EXPORT AVR_ArchXT_WDT : public AVR_WatchdogTimer {

public:

    explicit AVR_ArchXT_WDT(const AVR_ArchXT_WDT_Config& config);

    virtual bool init(AVR_Device& device) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

protected:

    virtual void timeout() override;

private:

    const AVR_ArchXT_WDT_Config& m_config;

};

#endif //__YASIMAVR_XT_WDT_H__
