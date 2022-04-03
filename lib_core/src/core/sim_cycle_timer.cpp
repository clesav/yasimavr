/*
 * sim_cycle_timer.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
		slot->timer->m_scheduled = false;
		delete slot;
	}

	m_timer_slots.clear();
}

void AVR_CycleManager::increment_cycle(cycle_count_t count)
{
	m_cycle += count;
}

int AVR_CycleManager::find_timer(AVR_CycleTimer* timer)
{
	for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
		TimerSlot* slot = *it;
		if (slot->timer == timer)
			return it - m_timer_slots.begin();
	}
	return -1;
}

void AVR_CycleManager::add_timer_to_queue(TimerSlot* slot)
{
	for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
		if ((slot->when < (*it)->when) || (*it)->paused) {
			m_timer_slots.insert(it, slot);
			return;
		}
	}
	m_timer_slots.push_back(slot);
}

AVR_CycleManager::TimerSlot* AVR_CycleManager::remove_timer_from_queue(AVR_CycleTimer* timer)
{
	int index = find_timer(timer);
	if (index >= 0) {
		TimerSlot* slot = m_timer_slots[index];
		m_timer_slots.erase(m_timer_slots.begin() + index);
		return slot;
	} else {
		return nullptr;
	}
}

void AVR_CycleManager::add_cycle_timer(AVR_CycleTimer* timer, cycle_count_t when)
{
	if (find_timer(timer) == -1) {
		TimerSlot* slot = new TimerSlot;
		slot->timer = timer;
		slot->when = when;
		slot->paused = false;
		slot->timer->m_scheduled = true;
		add_timer_to_queue(slot);
	}
}

void AVR_CycleManager::remove_cycle_timer(AVR_CycleTimer* timer)
{
	TimerSlot* slot = remove_timer_from_queue(timer);
	if (slot) {
		slot->timer->m_scheduled = false;
		delete slot;
	}
}

void AVR_CycleManager::reschedule_cycle_timer(AVR_CycleTimer* timer, cycle_count_t timer_when)
{
	TimerSlot* slot = remove_timer_from_queue(timer);
	if (slot) {
		slot->when = timer_when;
		add_timer_to_queue(slot);
	} else {
		add_cycle_timer(timer, timer_when);
	}
}

void AVR_CycleManager::pause_cycle_timer(AVR_CycleTimer* timer)
{
	TimerSlot* slot = remove_timer_from_queue(timer);
	if (slot) {
		slot->paused = true;
		slot->when = (slot->when > m_cycle) ? (slot->when - m_cycle) : 0;
		add_timer_to_queue(slot);
	}
}

void AVR_CycleManager::resume_cycle_timer(AVR_CycleTimer* timer)
{
	TimerSlot* slot = remove_timer_from_queue(timer);
	if (slot) {
		slot->paused = false;
		slot->when = slot->when + m_cycle;
		add_timer_to_queue(slot);
	}
}

void AVR_CycleManager::process_cycle_timers()
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
				add_timer_to_queue(slot);
			} else {
				slot->timer->m_scheduled = false;
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
