/*
 * sim_cycle_timer.cpp
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

#include "sim_cycle_timer.h"
#include <algorithm>
#include <stdexcept>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

// Flags for the m_state variable in CycleTimer
// Indicates that the timer is scheduled with the manager
#define STATE_SCHEDULED  0x01
// Indicates that the timer is 'processing' i.e. we are currently in its 'next' callback
#define STATE_PROCESSING 0x02
// Indicated that the timer is paused
#define STATE_PAUSED     0x04


CycleTimer::CycleTimer()
:m_manager(nullptr)
,m_state(0)
,m_when(INVALID_CYCLE)
{}


CycleTimer::~CycleTimer()
{
    if (m_state & STATE_SCHEDULED)
        m_manager->cancel(*this);
}


CycleTimer::CycleTimer(const CycleTimer& other)
:CycleTimer()
{
    *this = other;
}


CycleTimer& CycleTimer::operator=(const CycleTimer& other)
{
    if (m_state & STATE_SCHEDULED)
        m_manager->cancel(*this);

    m_manager = other.m_manager;

    if (other.m_manager)
        other.m_manager->copy_slot(other, *this);

    return *this;
}

/**
 * Initialisation of the timer by assigning it a manager and a clock domain.
 * Must be called before any operation on the timer.
 */
void CycleTimer::init(CycleManager& manager)
{
    m_manager = &manager;
}


[[noreturn]]
static void throw_uninitialised()
{
    throw std::runtime_error("Timer not initialised");
}


/**
   Schedule or reschedule a timer for call in 'delay' cycles
   \param delay delay from the current cycle, in cycle number
 */
void CycleTimer::delay(cycle_count_t count)
{
    if (count <= 0) {
        cancel();
        return;
    }

    if (!m_manager)
        throw_uninitialised();

    m_manager->delay(*this, count);
}


/**
   Remove this timer from the scheduling queue. No-op if not scheduled.
 */
void CycleTimer::cancel()
{
    if (m_state & STATE_SCHEDULED)
        m_manager->cancel(*this);
}


/**
   Pause this timer.

   The timer stays in the scheduling queue but won't be called until it's resumed.
   The remaining delay until the timer 'when' is conserved during the pause.
   \sa resume
 */
void CycleTimer::pause()
{
    if ((m_state & STATE_SCHEDULED) && !(m_state & STATE_PAUSED))
        m_manager->pause(*this);
}

/**
   Resume a paused timer.
   \sa pause
 */
void CycleTimer::resume()
{
    if (m_state & STATE_PAUSED)
        m_manager->resume(*this);
}

/**
 * Returns true if this timer is scheduled with a manager.
 */
bool CycleTimer::scheduled() const
{
    return m_state & STATE_SCHEDULED;
}

/**
 * Returns true if the timer is "processing" i.e. we are currently
 * in its "next" callback
 */
bool CycleTimer::processing() const
{
    return m_state & STATE_PROCESSING;
}

/**
 * Returns true if the timer is paused.
 */
bool CycleTimer::paused() const
{
    return m_state & STATE_PAUSED;
}

/**
   Returns the remaining delay before this timer is called, or -1 if the timer isn't scheduled.
 */
cycle_count_t CycleTimer::remaining_delay() const
{
    if (!m_state)
        return INVALID_CYCLE;

    cycle_count_t d;
    if (m_state & STATE_PAUSED)
        d = m_when;
    else if (m_when < m_manager->m_cycle)
        d = 0;
    else
        d = m_when - m_manager->m_cycle;

    return d;
}


//=======================================================================================

CycleManager::CycleManager()
:m_cycle(0)
,m_processing(false)
,m_processed_when(0)
{}


CycleManager::~CycleManager()
{
    for (auto t : m_timer_slots) {
        t->m_state = 0;
        t->m_manager = nullptr;
    }
}

/**
   Increment the cycle counter.
 */
void CycleManager::increment_cycle(cycle_count_t count)
{
    m_cycle += count;
}


void CycleManager::add_to_queue(CycleTimer& timer)
{
    //Add a timer slot to the queue
    //The timers are ordered in chronological order (in cycle count), the front being
    //the first timer to be called.
    //The inactive (i.e. paused) timers are placed after all the active timers.
    //Here, we use an insertion sort algorithm.
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        if ((timer.m_when < (*it)->m_when) || ((*it)->m_state & STATE_PAUSED)) {
            m_timer_slots.insert(it, &timer);
            return;
        }
    }
    m_timer_slots.push_back(&timer);
}


bool CycleManager::pop_from_queue(CycleTimer& timer)
{
    auto it = std::find(m_timer_slots.begin(), m_timer_slots.end(), &timer);
    if (it != m_timer_slots.end()) {
        m_timer_slots.erase(it);
        return true;
    } else {
        return false;
    }
}


void CycleManager::delay(CycleTimer& timer, cycle_count_t count)
{
    cycle_count_t ref_cycle = m_processing ? m_processed_when : m_cycle;

    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    timer.m_state &= ~STATE_PAUSED;
    timer.m_state |= STATE_SCHEDULED;
    timer.m_when = ref_cycle + count;

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


void CycleManager::cancel(CycleTimer& timer)
{
    if (timer.m_state & STATE_PROCESSING) {
        timer.m_state &= STATE_PROCESSING;
    } else {
        pop_from_queue(timer);
        timer.m_state = 0;
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
    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    if (!(timer.m_state & STATE_PAUSED)) {
        timer.m_state |= STATE_PAUSED;
        timer.m_when = (timer.m_when > m_cycle) ? (timer.m_when - m_cycle) : 0;
    }

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


void CycleManager::resume(CycleTimer& timer)
{
    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    if (timer.m_state & STATE_PAUSED) {
        timer.m_state &= ~STATE_PAUSED;
        timer.m_when = timer.m_when + m_cycle;
    }

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


/**
   Process the timers for the current cycle.
 */
void CycleManager::process_timers()
{
    m_processing = true;

    //Loops until either the timer queue is empty or the front timer is paused or its 'when' is in the future
    while(!m_timer_slots.empty()) {
        CycleTimer* t = m_timer_slots.front();

        //Paused timers are last in the queue. It means we have no more active timer.
        if ((t->m_when > m_cycle) || (t->m_state & STATE_PAUSED)) break;

        //Remove the timer from the front of the queue
        m_timer_slots.pop_front();
        t->m_state = STATE_PROCESSING;

        //Calling the timer's callback
        m_processed_when = t->m_when;
        t->next(m_processed_when);

        //return the timer to the queue if it had been rescheduled
        if (t->m_state & STATE_SCHEDULED)
            add_to_queue(*t);
        else
            t->m_state = 0;
    }

    m_processing = false;

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

    CycleTimer* t = m_timer_slots.front();
    if (t->m_state & STATE_PAUSED)
        return INVALID_CYCLE;
    else
        return t->m_when;
}


void CycleManager::copy_slot(const CycleTimer& src, CycleTimer& dst)
{
    dst.m_state = src.m_state;
    dst.m_when = src.m_when;

    if (dst.m_state)
        add_to_queue(dst);
}
