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


//=======================================================================================

AVR_CycleTimer::AVR_CycleTimer()
:m_manager(nullptr)
{}


AVR_CycleTimer::~AVR_CycleTimer()
{
    if (m_manager)
        m_manager->cancel(*this);
}


AVR_CycleTimer::AVR_CycleTimer(const AVR_CycleTimer& other)
{
    *this = other;
}


AVR_CycleTimer& AVR_CycleTimer::operator=(const AVR_CycleTimer& other)
{
    if (m_manager)
        m_manager->cancel(*this);

    if (other.m_manager)
        other.m_manager->copy_slot(other, *this);

    return *this;
}


struct AVR_CycleManager::TimerSlot {
    //Pointer to the timer
    AVR_CycleTimer* timer;

    //When the timer is running, it's the absolute cycle when the timer should be called
    //When the timer is paused, it's the remaining delay until a call
    cycle_count_t when;

    //Indicates if the timer is paused
    bool paused;
};


AVR_CycleManager::AVR_CycleManager()
:m_cycle(0)
{}


AVR_CycleManager::~AVR_CycleManager()
{
    //Destroys the cycle timer slots
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        TimerSlot* slot = *it;
        slot->timer->m_manager = nullptr;
        delete slot;
    }

    m_timer_slots.clear();
}


void AVR_CycleManager::increment_cycle(cycle_count_t count)
{
    m_cycle += count;
}


void AVR_CycleManager::add_to_queue(TimerSlot* slot)
{
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        if ((slot->when < (*it)->when) || (*it)->paused) {
            m_timer_slots.insert(it, slot);
            return;
        }
    }
    m_timer_slots.push_back(slot);
}


AVR_CycleManager::TimerSlot* AVR_CycleManager::pop_from_queue(AVR_CycleTimer& timer)
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


void AVR_CycleManager::schedule(AVR_CycleTimer& timer, cycle_count_t when)
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


void AVR_CycleManager::delay(AVR_CycleTimer& timer, cycle_count_t delay)
{
    if (delay > 0)
        schedule(timer, m_cycle + delay);
}


void AVR_CycleManager::cancel(AVR_CycleTimer& timer)
{
    TimerSlot* slot = pop_from_queue(timer);
    if (slot) {
        slot->timer->m_manager = nullptr;
        delete slot;
    }
}


void AVR_CycleManager::pause(AVR_CycleTimer& timer)
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


void AVR_CycleManager::resume(AVR_CycleTimer& timer)
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


void AVR_CycleManager::process_timers()
{
    //Loops until either the timer queue is empty or the front timer is paused or its 'when' is in the future
    while(!m_timer_slots.empty()) {
        TimerSlot* slot = m_timer_slots.front();
        //Paused timers are last in the queue. It means we have no more active timer.
        if (slot->paused) break;
        if (slot->when > m_cycle) {
            break;
        } else {
            //Removes the timer from the front of the queue
            m_timer_slots.pop_front();
            //Calling the timer next function
            cycle_count_t next_when = slot->timer->next(slot->when);
            //If the returned 'when' is not zero, we reschedule the timer
            //(the next 'when' might be in the past)
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


cycle_count_t AVR_CycleManager::next_when() const
{
    if (m_timer_slots.empty())
        return INVALID_CYCLE;

    TimerSlot* slot = m_timer_slots.front();
    if (slot->paused)
        return INVALID_CYCLE;
    else
        return slot->when;
}


void AVR_CycleManager::copy_slot(const AVR_CycleTimer& src, AVR_CycleTimer& dst)
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
