/*
 * sim_wdt.cpp
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

#include "sim_wdt.h"
#include "../core/sim_device.h"


//=======================================================================================

class AVR_WatchdogTimer::WDT_Timer : public AVR_CycleTimer {

public:

    explicit WDT_Timer(AVR_WatchdogTimer& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override {
        return m_ctl.wd_timer_next(when);
    }

private:

    AVR_WatchdogTimer& m_ctl;

};


//=======================================================================================

class AVR_WatchdogTimer::WDR_Sync_Timer : public AVR_CycleTimer {

public:

    explicit WDR_Sync_Timer(AVR_WatchdogTimer& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override {
        return m_ctl.wdr_sync_timer_next(when);
    }

private:

    AVR_WatchdogTimer& m_ctl;

};


//=======================================================================================
/*
 * Constructor of a generic watchdog timer
 */
AVR_WatchdogTimer::AVR_WatchdogTimer()
:AVR_Peripheral(AVR_IOCTL_WTDG)
,m_clk_factor(0)
,m_win_start(0)
,m_win_end(0)
,m_wdr_cycle(0)
,m_wdr_sync(false)
{
    m_wd_timer = new WDT_Timer(*this);
    m_wdr_sync_timer = new WDR_Sync_Timer(*this);
}

/*
 * Destructor of a generic watchdog timer
 */
AVR_WatchdogTimer::~AVR_WatchdogTimer()
{
    delete m_wd_timer;
    delete m_wdr_sync_timer;
}

/*
 * On a reset, cancel the two timers
 */
void AVR_WatchdogTimer::reset()
{
    m_win_start = 0;
    m_win_end = 0;
    m_wdr_sync = false;
    device()->remove_cycle_timer(m_wd_timer);
    device()->remove_cycle_timer(m_wdr_sync_timer);
}

/*
 * Sets the timer delay, and start the cycle timer if necessary
 */
void AVR_WatchdogTimer::set_timer(uint32_t wdr_win_start, uint32_t wdr_win_end, uint32_t clk_factor)
{
    bool was_enabled = (m_win_end > 0);
    m_clk_factor = clk_factor;
    m_win_start = wdr_win_start;
    m_win_end = wdr_win_end;

    if (m_win_end) {
        cycle_count_t elapsed = was_enabled ? (device()->cycle() - m_wdr_cycle) : 0;
        cycle_count_t wd_delay = m_clk_factor * m_win_end;
        if (wd_delay > elapsed)
            device()->reschedule_cycle_timer(m_wd_timer, m_wdr_cycle + wd_delay - elapsed);
        else
            timeout();
    }
    else {
        device()->remove_cycle_timer(m_wd_timer);
    }
}

/*
 * Handle the watchdog reset request by rescheduling the timer
 * The timer is only reset after the reset signal is synced to the watchdog clock
 * (3 cycles)
 * Ignores the request if there's already one being synced
 */
bool AVR_WatchdogTimer::ctlreq(uint16_t req, ctlreq_data_t* __unused)
{
    if (req == AVR_CTLREQ_WATCHDOG_RESET) {
        if (m_win_end && !m_wdr_sync) {
            m_wdr_sync = true;
            device()->add_cycle_timer(m_wdr_sync_timer, device()->cycle() + m_clk_factor * 3);
        }
        return true;
    }
    return false;
}

/*
 * Watchdog timeout notification.
 */
cycle_count_t AVR_WatchdogTimer::wd_timer_next(cycle_count_t when)
{
    timeout();

    m_wdr_cycle = when;

    if (m_win_end)
        return when + m_clk_factor * m_win_end;
    else
        return 0;
}

/*
 * Watchdog reset sync timeout notification.
 * handles the reset of the watchdog timer
 * triggers a timeout if the window start is set and the reset is too soon
 */
cycle_count_t AVR_WatchdogTimer::wdr_sync_timer_next(cycle_count_t when)
{
    if (m_win_start && when < (m_clk_factor * m_win_start + m_wdr_cycle))
        timeout();

    m_wdr_cycle = when;

    if (m_win_end) {
        cycle_count_t wd_when = when + m_clk_factor * m_win_end;
        device()->reschedule_cycle_timer(m_wd_timer, wd_when);
    }

    return 0;
}
