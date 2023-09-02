/*
 * sim_cycle_timer.h
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

#ifndef __YASIMAVR_CYCLE_TIMER_H__
#define __YASIMAVR_CYCLE_TIMER_H__

#include "sim_types.h"
#include <deque>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Abstract interface for timers that can register with the cycle manager and
 * be called at a certain cycle
 */

class CycleManager;

class AVR_CORE_PUBLIC_API CycleTimer {

public:

    CycleTimer();
    //Copy constructor
    CycleTimer(const CycleTimer& other);
    //Destructor: ensures the timer is removed from the manager
    virtual ~CycleTimer();

    inline bool scheduled() const { return !!m_manager; }

    //Callback from the cycle loop. 'when' is the current cycle
    //The returned value is the next 'when' cycle this timer wants to be called.
    //If it's less or equal than the 'when' argument, the timer is removed from the pool.
    //Note than there's no guarantee the method will be called exactly on the required 'when'.
    //The only guarantee is "called 'when' >= required 'when'"
    //The implementations must account for this.
    virtual cycle_count_t next(cycle_count_t when) = 0;

    //Copy assignment
    CycleTimer& operator=(const CycleTimer& other);

private:

    friend class CycleManager;

    //Pointer to the cycle manager when the timer is scheduled. Null when not scheduled.
    CycleManager* m_manager;

};


//=======================================================================================
/*
 * Class to manage the simulation cycle counter and cycle timers
 * Cycles are meant to represent one cycle of the MCU main clock though
 * the overall cycle-level accuracy of the simulation is not guaranteed.
 * It it a counter guaranteed to start at 0 and always increasing.
 */
class AVR_CORE_PUBLIC_API CycleManager {

public:

    CycleManager();
    ~CycleManager();

    cycle_count_t cycle() const;
    void increment_cycle(cycle_count_t count);

    //Adds a new timer and schedule it for call at 'when'
    void schedule(CycleTimer& timer, cycle_count_t when);

    void delay(CycleTimer& timer, cycle_count_t delay);

    //Remove a timer from the queue. No-op if the timer is not scheduled.
    void cancel(CycleTimer& timer);

    //Pause a timer for call. It means the timer stays in the queue but won't be called
    //until it's resumed. The delay until the timer 'when' is conserved during the pause.
    void pause(CycleTimer& timer);

    //Resumes a paused timer
    void resume(CycleTimer& timer);

    //Process the timers for the current cycle
    void process_timers();

    //Returns the next cycle 'when' where timers require to be processed
    //Returns INVALID_CYCLE if no timer to be processed
    cycle_count_t next_when() const;

    //no copy semantics
    CycleManager(const CycleManager&) = delete;
    CycleManager& operator=(const CycleManager&) = delete;

private:

    friend class CycleTimer;

    //Structure holding information on a cycle timer when it's in the cycle queue
    struct TimerSlot;
    std::deque<TimerSlot*> m_timer_slots;
    cycle_count_t m_cycle;

    //Utility method to add a timer in the cycle queue, conserving the order or 'when'
    //and paused timers last
    void add_to_queue(TimerSlot* slot);

    //Utility to remove a timer from the queue.
    TimerSlot* pop_from_queue(CycleTimer& timer);

    void copy_slot(const CycleTimer& src, CycleTimer& dst);

};

inline cycle_count_t CycleManager::cycle() const
{
    return m_cycle;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_CYCLE_TIMER_H__
