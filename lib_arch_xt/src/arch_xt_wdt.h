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

#include "arch_xt_globals.h"
#include "ioctrl_common/sim_wdt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Configuration structure for ArchXT_WDT
 */
struct ArchXT_WDTConfig {

    /// Frequency in Hertz of the clock used for the Watchdog Timer
    unsigned long clock_frequency;
    /// Watchdog timer delays
    std::vector<unsigned long> delays;
    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;

};


/**
   \brief Implementation of a Watchdog Timer for XT core series
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_WDT : public WatchdogTimer {

public:

    explicit ArchXT_WDT(const ArchXT_WDTConfig& config);

    virtual bool init(Device& device) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

protected:

    virtual void timeout() override;

private:

    const ArchXT_WDTConfig& m_config;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_WDT_H__
