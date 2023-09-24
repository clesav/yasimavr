/*
 * sim_wdt.h
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

#ifndef __YASIMAVR_IO_WDT_H__
#define __YASIMAVR_IO_WDT_H__

#include "../core/sim_cycle_timer.h"
#include "../core/sim_peripheral.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \brief Generic I/O controller that implements a watchdog timer.

   It combines a classic period feature with a window option such as on Mega0 series.

   The actual effect of the timeout is left to sub-classes by overriding timeout()

   The configuration via fuses is not supported at the moment.
 */
class AVR_CORE_PUBLIC_API WatchdogTimer : public Peripheral {

public:

    WatchdogTimer();
    virtual ~WatchdogTimer();

    virtual void reset() override;
    /// Override to handle the core request AVR_CTLREQ_WATCHDOG_RESET
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;

protected:

    /**
       Configuration of the timer.\n
       the timer is enabled for wdr_win_end > 0.
       If wdr_win_start > 0, the window feature is enabled.\n
       clk_factor is the ratio WDT_clock_freq / MCU_clock_freq.\n
     */
    void set_timer(uint32_t wdr_win_start, uint32_t wdr_win_end, uint32_t clk_factor);

    /**
       Callback to be reimplemented by sub-classes for handling the effects of a
       timeout.
     */
    virtual void timeout() = 0;

private:

    class WDT_Timer;
    class WDR_Sync_Timer;

    friend class WD_Timer;
    friend class WDR_Sync_Timer;

    uint32_t m_clk_factor;
    uint32_t m_win_start;
    uint32_t m_win_end;

    //Stores the cycle number of the last WDR.
    cycle_count_t m_wdr_cycle;

    WDT_Timer* m_wd_timer;

    //The WDR reset simulates a signal synchronisation to the watchdog clock
    //by waiting for 3 WDT clock cycles before resetting the timer
    //this flag indicates if the timer is for a WDT timeout or for WDR signal sync.
    WDR_Sync_Timer* m_wdr_sync_timer;

    cycle_count_t wd_timer_next(cycle_count_t when);
    cycle_count_t wdr_sync_timer_next(cycle_count_t when);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_IO_WDT_H__
