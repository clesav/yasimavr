/*
 * arch_xt_wdt.cpp
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

#include "arch_xt_wdt.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    (m_config.reg_base + offsetof(WDT_t, reg))

#define REG_OFS(reg) \
    offsetof(WDT_t, reg)


//=======================================================================================

/*
 * Constructor of a generic watchdog timer
 */
ArchXT_WDT::ArchXT_WDT(const ArchXT_WDTConfig& config)
:m_config(config)
{}

bool ArchXT_WDT::init(Device& device)
{
    bool status = WatchdogTimer::init(device);

    add_ioreg(REG_ADDR(CTRLA), WDT_PERIOD_gm | WDT_WINDOW_gm);
    //STATUS not implemented

    return status;
}

void ArchXT_WDT::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    unsigned long clk_factor = device()->frequency() / m_config.clock_frequency;
    uint8_t win_start_index = (data.value & WDT_WINDOW_gm) >> WDT_WINDOW_gp;
    uint8_t win_end_index = (data.value & WDT_PERIOD_gm) >> WDT_PERIOD_gp;
    unsigned long win_start = m_config.delays[win_start_index];
    unsigned long win_end = m_config.delays[win_end_index];
    set_timer(win_start, win_end, clk_factor);
}

void ArchXT_WDT::timeout()
{
    //Trigger the reset. Don't call reset() because we want the current
    //cycle to complete beforehand. The state of the device would be
    //inconsistent otherwise.
    ctlreq_data_t reqdata = { .data = Device::Reset_WDT };
    device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);
}
