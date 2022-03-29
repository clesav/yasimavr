/*
 * sim_timer.cpp
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

#include "sim_timer.h"


//=======================================================================================

AVR_PrescaledTimer::AVR_PrescaledTimer()
:m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_ps_max(1)
,m_ps_factor(0)
,m_ps_counter(0)
,m_delay(0)
,m_paused(false)
,m_updating(false)
,m_update_cycle(0)
,m_parent_timer(nullptr)
{}

AVR_PrescaledTimer::~AVR_PrescaledTimer()
{
	if (m_parent_timer)
		m_parent_timer->unregister_chained_timer(*this);

	for (AVR_PrescaledTimer* timer : m_chained_timers)
		timer->m_parent_timer = nullptr;
}

void AVR_PrescaledTimer::init(AVR_CycleManager& cycle_manager, AVR_DeviceLogger& logger)
{
	m_cycle_manager = &cycle_manager;
	m_logger = &logger;
}

void AVR_PrescaledTimer::reset()
{
	m_ps_max = 1;
	m_ps_factor = 0;
	m_ps_counter = 0;
	m_paused = false;
	m_delay = 0;
	if (!m_updating)
		m_cycle_manager->remove_cycle_timer(this);
}

void AVR_PrescaledTimer::set_prescaler(uint32_t ps_max, uint32_t ps_factor)
{
	if (!m_updating) update(m_cycle_manager->cycle());

	if (!ps_max) ps_max = 1;
	m_ps_max = ps_max;
	m_ps_factor = ps_factor;

	if (!ps_factor)
		m_ps_counter = 0;
	else
		m_ps_counter %= ps_max;

	if (!m_updating) reschedule();
}

void AVR_PrescaledTimer::set_paused(bool paused)
{
	if (!m_updating) update();
	m_paused = paused;
	if (!m_updating) reschedule();
}

void AVR_PrescaledTimer::set_timer_delay(uint32_t delay)
{
	if (!m_updating) update();
	m_delay = delay;
	if (!m_updating) reschedule();
}

void AVR_PrescaledTimer::reschedule()
{
	cycle_count_t when;

	if (m_parent_timer) {
		//Only the top timer in a chain needs be scheduled
		//with the cycle manager
		when = 0;
		m_parent_timer->reschedule();
	} else {
		when = calculate_when(m_cycle_manager->cycle());
	}

	if (when > 0)
		m_cycle_manager->reschedule_cycle_timer(this, when);
	else if (scheduled())
		m_cycle_manager->remove_cycle_timer(this);
}

void AVR_PrescaledTimer::update(cycle_count_t when)
{
	if (m_updating) return;
	m_updating = true;

	if (when == INVALID_CYCLE)
		when = m_cycle_manager->cycle();

	if (m_parent_timer)
		m_parent_timer->update(when);
	else
		update_timer(when);

	m_updating = false;
}

void AVR_PrescaledTimer::update_timer(cycle_count_t when)
{
	//Number of clock cycles since the last update
	cycle_count_t dt = when - m_update_cycle;

	//Checking for dt != 0 ensures ticks are generated only once
	if (dt) {
		process_cycles(dt);
		//Remember the update cycle number for the next update
		m_update_cycle = when;
	}
}

void AVR_PrescaledTimer::process_cycles(cycle_count_t cycles)
{
	//This part generates the prescaler ticks corresponding to the update interval
	//If ticks occurred in the interval, we check if there's enough to trigger the timeout
	//If so, we raise the signal so that the peripheral handles the event.
	//(the peripheral may change the timeout delay and prescaler settings on the fly)
	//We loop by consuming the clock cycles we're catching up with and which generated the ticks
	//We exit the loop once there are not enough clock cycles to generate any tick, or we've
	//been disabled in the signal hook
	DEBUG_LOG(*m_logger, "Prescaled timer processing cycles dt=%d", cycles);
	while (m_ps_factor && !m_paused) {
		//Calculate the nb of prescaler ticks that occurred in the update interval
		cycle_count_t ticks = (cycles + m_ps_counter % m_ps_factor) / m_ps_factor;

		//Process the chained timers
		for (AVR_PrescaledTimer* timer : m_chained_timers)
			timer->process_cycles(ticks);

		//If not enough cycles to generate any tick or the timer is disabled,
		//just update the prescaler counter and leave the loop
		if (!ticks || !m_delay) {
			m_ps_counter = (cycles + m_ps_counter) % m_ps_max;
			break;
		}

		//Is it enough to reach the timeout delay ?
		bool timeout = (ticks >= m_delay);

		//Exact number of clock cycle required to generate the available ticks,
		//limiting it to the timeout delay, so that we can loop with the remaining amount
		cycle_count_t ticks_dt = (timeout ? m_delay : ticks) * m_ps_factor;
		cycles -= ticks_dt;

		//Update the prescaler counter accordingly
		m_ps_counter = (ticks_dt + m_ps_counter) % m_ps_max;

		DEBUG_LOG(*m_logger, "Prescaled timer generating %d ticks, delay=%d", ticks, m_delay);

		//Raise the signal to inform the parent peripheral of ticks to consume
		//Decrement the delay by the number of ticks
		signal_data_t sigdata = { .sigid = 0 };
		if (timeout) {
			sigdata.index = 1;
			sigdata.data = m_delay;
			m_delay = 0;
		} else {
			sigdata.index = 0;
			sigdata.data = (uint32_t) ticks;
			m_delay -= ticks;
		}
		m_signal.raise(sigdata);

	}
}

/*
 * Methods calculating when the cycle timer should timeout
 * This is calculated by returning the smallest number of cycles
 * required to reach the timer delay (if non-zero and factoring for
 * the prescaler) of this timer or any of the chained timers.
 * If the timer has a parent, the parent prescaler is factored in.
 */
cycle_count_t AVR_PrescaledTimer::calculate_when(cycle_count_t when)
{
	cycle_count_t delay = calculate_delay();
	return (delay > 0) ? (when + delay) : 0;
}

cycle_count_t AVR_PrescaledTimer::calculate_delay()
{
	cycle_count_t cycles = 0;

	if (!m_paused && m_ps_factor) {
		if (m_delay)
			cycles = convert_ticks_to_cycles(m_delay);

		for (AVR_PrescaledTimer* timer : m_chained_timers) {
			cycle_count_t c = timer->calculate_delay();
			if (c > 0 && (cycles == 0 || c < cycles))
				cycles = c;
		}
	}

	return cycles;
}

cycle_count_t AVR_PrescaledTimer::convert_ticks_to_cycles(cycle_count_t ticks)
{
	cycle_count_t cycles = 0;

	if (m_ps_factor) {
		cycle_count_t tick_remainder = m_ps_factor - m_ps_counter % m_ps_factor;
		cycles = (ticks - 1) * m_ps_factor + tick_remainder;

		//If the timer has a parent, input cycles for this timer are ticks of the
		//parent timer prescaler so we need the parent to convert these as well.
		if (m_parent_timer)
			cycles = m_parent_timer->convert_ticks_to_cycles(cycles);
	}

	return cycles;
}

/*
 * Callback from the cycle timer processing. Update the counters and reschedule for the next timeout
 */
cycle_count_t AVR_PrescaledTimer::next(cycle_count_t when)
{
	update(when);
	return calculate_when(when);
}

/*
 * Methods adding or removing a chained timer
 */
void AVR_PrescaledTimer::register_chained_timer(AVR_PrescaledTimer& timer)
{
	if (!m_updating) update();

	timer.m_parent_timer = this;
	m_chained_timers.push_back(&timer);

	if (!m_updating) reschedule();
}

void AVR_PrescaledTimer::unregister_chained_timer(AVR_PrescaledTimer& timer)
{
	if (!m_updating) update();

	timer.m_parent_timer = nullptr;

	for (auto it = m_chained_timers.begin(); it != m_chained_timers.end(); ++it) {
		if (*it == &timer) {
			m_chained_timers.erase(it);
			break;
		}
	}

	if (!m_updating) reschedule();
}
