/*
 * arch_xt_wdt.cpp
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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
#include "arch_xt_device.h"

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
:Peripheral(AVR_IOCTL_WDT)
,m_config(config)
,m_wdt_timer(*this, &ArchXT_WDT::timeout)
,m_wdr_sync_timer(*this, &ArchXT_WDT::wdr_sync_timer_next)
,m_first_wdr(true)
{}


bool ArchXT_WDT::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), WDT_PERIOD_gm | WDT_WINDOW_gm);
    //SYNCBUSY not implemented
    add_ioreg(REG_ADDR(STATUS), WDT_LOCK_bm);

    m_wdt_timer.init(*device.cycle_manager());
    m_wdr_sync_timer.init(*device.cycle_manager());

    return status;
}


void ArchXT_WDT::reset(int)
{
    m_wdt_timer.cancel();
    m_wdr_sync_timer.cancel();
    m_first_wdr = true;

    //Obtain the pointer to the fuse block in NVM
    ctlreq_data_t req = { .index = ArchXT_Core::NVM_Fuses };
    if (!device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return;

    //Read the value of the WDTCFG fuse and copy it into the CTRLA register
    NonVolatileMemory* fuses = req.data.as_ptr<NonVolatileMemory>();
    uint8_t fuse_wdtcfg = (*fuses)[offsetof(FUSE_t, WDTCFG)];
    write_ioreg(REG_ADDR(CTRLA), fuse_wdtcfg);

    //If the WDT is enabled by the fuse, set the LOCK bit and start the timer
    uint8_t period_index = EXTRACT_F(fuse_wdtcfg, WDT_PERIOD);
    if (period_index) {
        SET_IOREG(STATUS, WDT_LOCK);
        auto [period, window] = calculate_delays(fuse_wdtcfg);
        m_wdt_timer.delay(period + window);
    }
}

/*
 * Handle the watchdog reset request by rescheduling the timer
 * The timer is only reset after the reset signal is synced to the watchdog clock
 * (3 cycles)
 * Ignores the request if there's already one being synced
 */
bool ArchXT_WDT::ctlreq(ctlreq_id_t req, ctlreq_data_t*)
{
    if (req == AVR_CTLREQ_WATCHDOG_RESET) {
        if (READ_IOREG_F(CTRLA, WDT_PERIOD) && !m_wdr_sync_timer.scheduled())
            m_wdr_sync_timer.delay(3);

        return true;
    }
    return false;
}


void ArchXT_WDT::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        //If the LOCK bit is set, reject any modification
        if (TEST_IOREG(STATUS, WDT_LOCK)) {
            write_ioreg(REG_ADDR(CTRLA), data.old);
            return;
        }

        auto [period, window] = calculate_delays(data.value);
        if (period) {
            if (!m_wdt_timer.scheduled())
                m_first_wdr = true;
            m_wdt_timer.delay(period + window);
        } else {
            m_wdt_timer.cancel();
        }
    }

    else if (reg_ofs == REG_OFS(STATUS)) {
        //Prevent clearing of the LOCK bit
        if (EXTRACT_B(data.negedge(), WDT_LOCK))
            SET_IOREG(STATUS, WDT_LOCK);
    }
}


std::tuple<cycle_count_t, cycle_count_t> ArchXT_WDT::calculate_delays(uint8_t reg_value)
{
    uint8_t reg_period = EXTRACT_F(reg_value, WDT_PERIOD);
    unsigned long p = (reg_period <= 11) ? 1 << (reg_period + 2) : 0;
    cycle_count_t period = (device()->frequency() * p) / m_config.clock_frequency;

    uint8_t reg_window = EXTRACT_F(reg_value, WDT_WINDOW);
    unsigned long w = (reg_window <= 11) ? 1 << (reg_window + 2) : 0;
    cycle_count_t window = (device()->frequency() * w) / m_config.clock_frequency;

    return std::tuple<cycle_count_t, cycle_count_t>(period, window);
}


void ArchXT_WDT::timeout()
{
    //Trigger the reset. Don't call reset() because we want the current
    //cycle to complete beforehand. The state of the device would be
    //inconsistent otherwise.
    ctlreq_data_t reqdata = { .data = Device::Reset_WDT };
    device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);
}


void ArchXT_WDT::wdr_sync_timer_next()
{
    //End of the WDR synchronisation.
    //If the window parameter is used, check if we're too early.
    //As per the datasheet, the first WDR after a reset is ignored.
    auto [period, window] = calculate_delays(READ_IOREG(CTRLA));
    if (window && (m_wdt_timer.remaining_delay() > period) && !m_first_wdr) {
        timeout();
    } else {
        m_wdt_timer.delay(period + window);
        m_first_wdr = false;
    }
}
