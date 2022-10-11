/*
 * sim_timer.h
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

#ifndef __YASIMAVR_IO_TIMER_H__
#define __YASIMAVR_IO_TIMER_H__

#include "../core/sim_cycle_timer.h"
#include "../core/sim_signal.h"
#include "../core/sim_device.h"

//=======================================================================================
/*
 * Generic implementation of a clock cycle timer, used by the peripherals TCx, WDT, RTC
 * It is structured with two consecutive stages:
 *  - Prescaler
 *  - Timer
 *  The prescaler works as a counter of simulated clock cycles, starting at 0,
 *  wrapping at 'ps_max', and generating timer 'ticks' once every 'ps_factor' cycles.
 *  The timer generates a timeout signal after a delay given in prescaler ticks.
 *  The class doesn't not have any tick counter as such. it only generates ticks that user objects
 *  can use to increment or decrement a counter. The meaning of the timeout signal is also left
 *  to users.
 *  The timeout is transmitted though a AVR_Signal, available via 'signal()' and raised in 2 ways:
 *   - When the programmed timeout delay is reached
 *   - When 'update()' is called, and enough clock cycles have passed, resulting in at least one tick.
 *  If the nb of ticks is enough to reach the set delay, the signal data index is set to 1.
 *  Otherwise, data.index is set to 0 and data.u is set to the available tick count.
 *  Timers can be daisy-chained, so that the prescaler output of a timer feeds into the
 *  prescaler of another.
 */
class AVR_PrescaledTimer : public AVR_CycleTimer {

public:

    AVR_PrescaledTimer();
    virtual ~AVR_PrescaledTimer();

    //Initialise the timer, must be called once during initialisation phases
    void init(AVR_CycleManager& cycle_manager, AVR_DeviceLogger& logger);
    //Reset the timer. Both stages are reset and disabled
    void reset();
    //Configure the prescaler:
    // - ps_max is the maximum value of the prescaler counter, making
    //   the prescaler counter wrap to 0
    // - ps_factor is the prescaler factor to generate ticks
    //   if ps_factor = 0, the prescaler and timeout delay are disabled and reset
    void set_prescaler(uint32_t ps_max, uint32_t ps_factor);
    //Getter for ps_factor
    uint32_t prescaler_factor() const;
    //Sets the timeout delay to generate a event
    //If delay = 0, the timeout delay is disabled and reset
    //The prescaler is not affected by this setting
    void set_timer_delay(uint32_t delay);
    //Getter for timer_delay
    uint32_t timer_delay() const;
    //Pauses the timer
    //If paused, the prescaler and timeout delay are frozen but not reset
    void set_paused(bool paused);
    //Update the timer to catchup with the 'when' cycle
    //Ticks may be generated and the signal may be raised if enough cycles have passed
    void update(cycle_count_t when = INVALID_CYCLE);

    //Callback override from AVR_CycleTimer
    virtual cycle_count_t next(cycle_count_t when) override;

    //Returns the signal that is raised with ticks
    AVR_Signal& signal();

    void register_chained_timer(AVR_PrescaledTimer& timer);
    void unregister_chained_timer(AVR_PrescaledTimer& timer);

    static int ticks_to_event(int counter, int event, int wrap);

private:

    AVR_CycleManager* m_cycle_manager;
    AVR_DeviceLogger* m_logger;

    //***** Prescaler management *****
    uint32_t m_ps_max;                  //Max value of the prescaler
    uint32_t m_ps_factor;               //Prescaler division factor (Tick period / Clock period)
    uint32_t m_ps_counter;              //Prescaler counter

    //***** Delay management *****
    uint32_t m_delay;                   //Delay until the next timeout in ticks

    //***** Update management *****
    bool m_paused;                      //Boolean indicating if the timer is paused
    bool m_updating;                    //Boolean used to avoid infinite updating reentrance
    cycle_count_t m_update_cycle;       //Cycle number of the last update
    AVR_Signal m_signal;                //Signal raised for processing ticks

    //***** Timer chain management *****
    std::vector<AVR_PrescaledTimer*> m_chained_timers;
    AVR_PrescaledTimer* m_parent_timer;

    void reschedule();
    void update_timer(cycle_count_t when);
    void process_cycles(cycle_count_t cycles);

    cycle_count_t calculate_when(cycle_count_t when);
    cycle_count_t calculate_delay();
    cycle_count_t convert_ticks_to_cycles(cycle_count_t ticks);

};

inline uint32_t AVR_PrescaledTimer::prescaler_factor() const
{
    return m_ps_factor;
}

inline uint32_t AVR_PrescaledTimer::timer_delay() const
{
    return m_delay;
}

inline AVR_Signal& AVR_PrescaledTimer::signal()
{
    return m_signal;
}

#endif //__YASIMAVR_IO_TIMER_H__
