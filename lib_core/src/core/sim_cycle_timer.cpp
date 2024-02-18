/*
 * sim_cycle_timer.cpp
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

#include "sim_cycle_timer.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

struct CycleManager::TimerSlot {
    //Pointer to the timer
    CycleTimer* timer;

    //When the timer is running, it's the absolute cycle when the timer should be called
    //When the timer is paused, it's the remaining delay until a call
    cycle_count_t when;

    //Indicates if the timer is paused
    bool paused;
};


//=======================================================================================

CycleTimer::CycleTimer()
:m_manager(nullptr)
{}


CycleTimer::~CycleTimer()
{
    if (m_manager)
        m_manager->cancel(*this);
}


CycleTimer::CycleTimer(const CycleTimer& other)
{
    *this = other;
}


CycleTimer& CycleTimer::operator=(const CycleTimer& other)
{
    if (m_manager)
        m_manager->cancel(*this);

    if (other.m_manager)
        other.m_manager->copy_slot(other, *this);

    return *this;
}

/**
   Returns true if this timer is scheduled and paused
 */
bool CycleTimer::paused() const
{
    if (!m_manager)
        return false;

    auto slot = m_manager->get_slot(*this);
    return slot->paused;
}


/**
   Returns the remaining delay before this timer is called, or -1 if the timer isn't scheduled.
 */
cycle_count_t CycleTimer::remaining_delay() const
{
    if (!m_manager)
        return INVALID_CYCLE;

    auto slot = m_manager->get_slot(*this);
    if (slot->paused)
        return slot->when;
    else if (slot->when < m_manager->cycle())
        return 0;
    else
        return slot->when - m_manager->cycle();
}


//=======================================================================================

CycleManager::CycleManager()
:m_cycle(0)
{}


CycleManager::~CycleManager()
{
    //Destroys the cycle timer slots
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        TimerSlot* slot = *it;
        slot->timer->m_manager = nullptr;
        delete slot;
    }

    m_timer_slots.clear();
}

/**
   Increment the cycle counter.
 */
void CycleManager::increment_cycle(cycle_count_t count)
{
    m_cycle += count;
}


void CycleManager::add_to_queue(TimerSlot* slot)
{
    //Add a timer slot to the queue
    //The timers are ordered in chronological order (in cycle count), the front being
    //the first timer to be called.
    //The inactive (i.e. paused) timers are placed after all the active timers.
    //Here, we use an insertion sort algorithm.
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        if ((slot->when < (*it)->when) || (*it)->paused) {
            m_timer_slots.insert(it, slot);
            return;
        }
    }
    m_timer_slots.push_back(slot);
}


CycleManager::TimerSlot* CycleManager::pop_from_queue(CycleTimer& timer)
{
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        TimerSlot* slot = *it;
        if (slot->timer == &timer) {
            m_timer_slots.erase(it);
            return slot;
        }
    }
    return nullptr;
}


CycleManager::TimerSlot* CycleManager::get_slot(const CycleTimer& timer) const
{
    for (auto slot : m_timer_slots) {
        if (slot->timer == &timer)
            return slot;
    }
    return nullptr;
}


/**
   Schedule or reschedule a timer for call at 'when'.
   \param timer timer to schedule
   \param when absolute cycle number when the timer should be called
 */
void CycleManager::schedule(CycleTimer& timer, cycle_count_t when)
{
    if (when < 0) return;

    TimerSlot* slot = pop_from_queue(timer);
    if (slot) {
        slot->when = when;
    } else {
        slot = new TimerSlot({ &timer, when, false });
        timer.m_manager = this;
    }
    add_to_queue(slot);
}


/**
   Schedule or reschedule a timer for call in 'delay' cycles
   \param timer timer to schedule
   \param delay delay from the current cycle number
 */
void CycleManager::delay(CycleTimer& timer, cycle_count_t delay)
{
    if (delay > 0)
        schedule(timer, m_cycle + delay);
}


/**
   Remove a timer from the queue. No-op if the timer is not scheduled.
 */
void CycleManager::cancel(CycleTimer& timer)
{
    TimerSlot* slot = pop_from_queue(timer);
    if (slot) {
        slot->timer->m_manager = nullptr;
        delete slot;
    }
}


/**
   Pause a timer.

   The timer stays in the queue but won't be called until it's resumed.
   The remaining delay until the timer 'when' is conserved during the pause.
   \sa resume
 */
void CycleManager::pause(CycleTimer& timer)
{
    TimerSlot* slot = pop_from_queue(timer);
    if (slot) {
        if (!slot->paused) {
            slot->paused = true;
            slot->when = (slot->when > m_cycle) ? (slot->when - m_cycle) : 0;
        }
        add_to_queue(slot);
    }
}

/**
   Resume a paused timer.
   \sa pause
 */
void CycleManager::resume(CycleTimer& timer)
{
    TimerSlot* slot = pop_from_queue(timer);
    if (slot) {
        if (slot->paused) {
            slot->paused = false;
            slot->when = slot->when + m_cycle;
        }
        add_to_queue(slot);
    }
}


/**
   Process the timers for the current cycle.
 */
void CycleManager::process_timers()
{
    //Loops until either the timer queue is empty or the front timer is paused or its 'when' is in the future
    while(!m_timer_slots.empty()) {
        TimerSlot* slot = m_timer_slots.front();
        //Paused timers are last in the queue. It means we have no more active timer.
        if (slot->paused) break;
        if (slot->when > m_cycle) {
            break;
        } else {
            //Remove the timer from the front of the queue
            m_timer_slots.pop_front();
            //Calling the timer
            cycle_count_t next_when = slot->timer->next(slot->when);
            //If the returned 'when' is greater than zero, reschedule the timer
            //(the next 'when' might be in the past)
            //If the returned 'when' is negative or zero, discard the timer
            if (next_when > 0) {
                //Ensure the 'when' always increments
                if (next_when <= slot->when)
                    next_when = slot->when + 1;
                slot->when = next_when;
                add_to_queue(slot);
            } else {
                slot->timer->m_manager = nullptr;
                delete slot;
            }
        }
    }
}

/**
   \return the next cycle at which a timer is scheduled to be called,
   or INVALID_CYCLE if no timer is scheduled or all scheduled timers are
   paused.
 */
cycle_count_t CycleManager::next_when() const
{
    if (m_timer_slots.empty())
        return INVALID_CYCLE;

    TimerSlot* slot = m_timer_slots.front();
    if (slot->paused)
        return INVALID_CYCLE;
    else
        return slot->when;
}


void CycleManager::copy_slot(const CycleTimer& src, CycleTimer& dst)
{
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        TimerSlot* src_slot = *it;
        if (src_slot->timer == &src) {
            TimerSlot* dst_slot = new TimerSlot( { &dst, src_slot->when, src_slot->paused });
            dst.m_manager = this;
            add_to_queue(dst_slot);
            return;
        }
    }
}
