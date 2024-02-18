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

class CycleManager;

/**
   Abstract interface for timers that can register with the cycle manager and
   be scheduled to be called at a given cycle.
 */
class AVR_CORE_PUBLIC_API CycleTimer {

public:

    CycleTimer();
    CycleTimer(const CycleTimer& other);
    virtual ~CycleTimer();

    /// Returns true if this timer is scheduled with a manager.
    inline bool scheduled() const { return !!m_manager; }

    bool paused() const;

    cycle_count_t remaining_delay() const;

    /**
       \brief Callback from the cycle loop.

       \note there's no guarantee the method will be called exactly on the required 'when' cycle.
       The only guarantee is "called 'when' <= 'current cycle'", the implementations must account for this.

       \param when current 'when' cycle, at which the timer was scheduled
       \return the next 'when' the timer requires to be called at.

       \note The next 'when' can be in the 'past' (i.e. <= 'current cycle').
       In this case, the timer will be called again within the same cycle with the given next 'when'.
       The only constraint is that it must be greater than the previous 'when'.
       If it's negative or zero, the timer is removed from the queue.
     */
    virtual cycle_count_t next(cycle_count_t when) = 0;

    CycleTimer& operator=(const CycleTimer& other);

private:

    friend class CycleManager;

    /// Pointer to the cycle manager when the timer is scheduled. Null when not scheduled.
    CycleManager* m_manager;

};


//=======================================================================================
/**
   Class to manage the simulation cycle counter and cycle timers

   Cycles are meant to represent one cycle of the MCU main clock though
   the overall cycle-level accuracy of the simulation is not guaranteed.
   It it a counter guaranteed to start at 0 and always increasing.
 */
class AVR_CORE_PUBLIC_API CycleManager {

public:

    CycleManager();
    ~CycleManager();

    cycle_count_t cycle() const;
    void increment_cycle(cycle_count_t count);

    void schedule(CycleTimer& timer, cycle_count_t when);

    void delay(CycleTimer& timer, cycle_count_t delay);

    void cancel(CycleTimer& timer);

    void pause(CycleTimer& timer);

    void resume(CycleTimer& timer);

    void process_timers();

    cycle_count_t next_when() const;

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

    TimerSlot* get_slot(const CycleTimer& timer) const;

};

/// Returns the current cycle
inline cycle_count_t CycleManager::cycle() const
{
    return m_cycle;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_CYCLE_TIMER_H__
