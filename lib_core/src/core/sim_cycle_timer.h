/*
 * sim_cycle_timer.h
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

    void init(CycleManager& manager);

    inline CycleManager* manager() const { return m_manager; }

    void delay(cycle_count_t count);
    void cancel();
    void pause();
    void resume();

    bool scheduled() const;
    bool processing() const;
    bool paused() const;

    cycle_count_t remaining_delay() const;

    /**
       \brief Callback from the cycle loop.

       \note there's no guarantee the method will be called exactly on the required 'when' cycle.
       The only guarantee is "called 'when' <= 'current cycle'", the implementations must account for this.

       \param when current 'when' cycle, at which the timer was scheduled
     */
    virtual void next(cycle_count_t when) = 0;

    CycleTimer& operator=(const CycleTimer& other);

private:

    friend class CycleManager;

    CycleManager* m_manager;
    uint8_t m_state;
    cycle_count_t m_when;

};


template<class C>
class BoundFunctionCycleTimer : public CycleTimer {

public:

    using bound_fct_t = void(C::*)(cycle_count_t);
    using bound_noarg_fct_t = void(C::*)(void);

    constexpr BoundFunctionCycleTimer(C& _c, bound_fct_t _f) : CycleTimer(), c(_c), arg(true), f_arg(_f) {}
    constexpr BoundFunctionCycleTimer(C& _c, bound_noarg_fct_t _f) : CycleTimer(), c(_c), arg(false), f_noarg(_f) {}

    virtual void next(cycle_count_t when) override final
    {
        if (arg)
            (c.*f_arg)(when);
        else
            (c.*f_noarg)();
    }

private:

    C& c;
    bool arg;

    union {
        const bound_fct_t f_arg;
        const bound_noarg_fct_t f_noarg;
    };

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

    void process_timers();

    cycle_count_t next_when() const;

    CycleManager(const CycleManager&) = delete;
    CycleManager& operator=(const CycleManager&) = delete;

private:

    friend class CycleTimer;

    //Structure holding information on a cycle timer when it's in the cycle queue
    std::deque<CycleTimer*> m_timer_slots;
    cycle_count_t m_cycle;
    bool m_processing;
    cycle_count_t m_processed_when;

    //Utility method to add a timer in the cycle queue, conserving the order or 'when'
    //and paused timers last
    void add_to_queue(CycleTimer& timer);

    //Utility to remove a timer from the queue.
    bool pop_from_queue(CycleTimer& timer);

    void copy_slot(const CycleTimer& src, CycleTimer& dst);

    void delay(CycleTimer& timer, cycle_count_t count);
    void cancel(CycleTimer& timer);
    void pause(CycleTimer& timer);
    void resume(CycleTimer& timer);

};


/// Returns the current cycle
inline cycle_count_t CycleManager::cycle() const
{
    return m_cycle;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_CYCLE_TIMER_H__
