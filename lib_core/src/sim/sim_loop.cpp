/*
 * sim_loop.cpp
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

#include "sim_loop.h"
#include "../core/sim_debug.h"
#include <unistd.h>
#include <chrono>


//=======================================================================================

#define MIN_SLEEP_THRESHOLD		200

typedef std::chrono::time_point<std::chrono::steady_clock> time_pt;

static uint64_t get_timestamp_usecs(time_pt origin)
{
	const time_pt stamp = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(stamp - origin).count();
}


const char* StateNames[4] = { "Run", "Step", "Stop", "Done" };


//=======================================================================================

AVR_AbstractSimLoop::Hook::Hook(AVR_AbstractSimLoop& simloop)
:m_simloop(simloop)
{}

AVR_AbstractSimLoop::Hook::Hook(AVR_AbstractSimLoop& simloop, cycle_count_t when)
:m_simloop(simloop)
{
	m_simloop.m_device.add_cycle_timer(this, when);
}

cycle_count_t AVR_AbstractSimLoop::Hook::next(cycle_count_t when)
{
	bool stop = false;
	cycle_count_t next_when = next(when, &stop);
	if (stop)
		m_simloop.m_state = State_Stopped;

	return next_when;
}


//=======================================================================================

AVR_AbstractSimLoop::AVR_AbstractSimLoop(AVR_Device& device)
:m_device(device)
,m_state(State_Running)
{
	m_device.init(m_cycle_manager);
}

AVR_AbstractSimLoop::~AVR_AbstractSimLoop()
{}

void AVR_AbstractSimLoop::run_device(cycle_count_t cycle_limit)
{
	cycle_count_t cycle_delta = m_device.exec_cycle();

	m_cycle_manager.process_cycle_timers();

	AVR_Device::State dev_state = m_device.state();

	if (dev_state == AVR_Device::State_Sleeping) {

		cycle_count_t next_timer_cycle = m_device.cycle_manager().next_when();

		if (next_timer_cycle == INVALID_CYCLE || next_timer_cycle >= cycle_limit)
			m_state = State_Stopped;

		else if (next_timer_cycle > m_cycle_manager.cycle())
			cycle_delta = next_timer_cycle - m_cycle_manager.cycle();

	}

	else if (dev_state >= AVR_Device::State_Done)
		m_state = State_Done;

	else if (dev_state >= AVR_Device::State_Stopped)
		m_state = State_Stopped;

	m_cycle_manager.increment_cycle(cycle_delta);
}


//=======================================================================================

AVR_SimLoop::AVR_SimLoop(AVR_Device& device)
:AVR_AbstractSimLoop(device)
,m_fast_mode(false)
{}


void AVR_SimLoop::run(cycle_count_t nbcycles)
{
	#define CYCLE_PER_TICK (m_device.frequency() / CLOCKS_PER_SEC)

	if (m_state == State_Done) return;

	if (!m_fast_mode && device().frequency() == 0) {
		ERROR_LOG(m_device.logger(), "Cannot run in realtime mode, MCU frequency not set.", "");
		m_state = State_Done;
		return;
	}

	if (m_device.state() < AVR_Device::State_Running) {
		ERROR_LOG(m_device.logger(), "Device not initialised or firmware not loaded", "");
		m_state = State_Done;
		return;
	}

	m_state = State_Running;

	const time_pt clock_start = std::chrono::steady_clock::now();
	const cycle_count_t cycle_start = m_cycle_manager.cycle();
	cycle_count_t final_cycle = nbcycles ? (cycle_start + nbcycles) : LLONG_MAX;

	while (m_cycle_manager.cycle() < final_cycle) {

		run_device(final_cycle);

		if (m_state >= State_Stopped)
			break;

		if (!m_fast_mode) {
			int64_t sim_deadline_us = ((m_cycle_manager.cycle() - cycle_start) * 1000000L) / m_device.frequency();
			int64_t curr_time_us = get_timestamp_usecs(clock_start);
			int64_t sleep_time_us = sim_deadline_us - curr_time_us;
			//DEBUG_LOG(m_device.logger(), "LOOP : Sim T=%dus, CPU T=%dus, Sleep T=%dus", sim_deadline_us, curr_time_us, sleep_time_us);
			if (sleep_time_us > MIN_SLEEP_THRESHOLD) {
				//WARNING_LOG(m_device.logger(), "LOOP : Sleeping %dus", sleep_time_us);
				usleep(sleep_time_us);
			}
		}
	}

	if (m_state < State_Done)
		m_state = State_Stopped;

}

void AVR_SimLoop::set_fast_mode(bool fast)
{
	m_fast_mode = fast;
}


//=======================================================================================

AVR_AsyncSimLoop::AVR_AsyncSimLoop(AVR_Device& device)
:AVR_AbstractSimLoop(device)
,m_cycling_enabled(false)
,m_cycle_wait(false)
{}

void AVR_AsyncSimLoop::run()
{
	if (m_state == State_Done) return;

	set_state(State_Stopped);

	std::unique_lock<std::mutex> cv_lock(m_cycle_mutex);
	cv_lock.unlock();

	while (true) {

		cv_lock.lock();

		while (!m_cycling_enabled || m_state == State_Stopped) {
			m_cycle_wait = true;
			m_sync_cv.notify_all();
			m_cycle_cv.wait(cv_lock);
			m_cycle_wait = false;
		}

		cv_lock.unlock();

		if (m_state == State_Running || m_state == State_Step) {
			run_device(LLONG_MAX);

			if (m_state == State_Step)
				set_state(State_Stopped);
		}

		else if (m_state == State_Done) {
			cv_lock.lock();
			m_sync_cv.notify_all();
			//No unlocking yet, to ensure we're off the cycle loop.
			//The unlocking is done by the destructor of 'cv_lock' on leaving run()
			break;
		}
	}
}

bool AVR_AsyncSimLoop::start_transaction()
{
	std::unique_lock<std::mutex> lock(m_cycle_mutex);
	m_cycling_enabled = false;
	while (!m_cycle_wait && m_state != State_Done)
		m_sync_cv.wait(lock);
	return m_state != State_Done;
}

void AVR_AsyncSimLoop::end_transaction()
{
	std::unique_lock<std::mutex> lock(m_cycle_mutex);
	m_cycling_enabled = true;
	m_cycle_cv.notify_all();
}

void AVR_AsyncSimLoop::loop_continue()
{
	set_state(State_Running);
}

void AVR_AsyncSimLoop::loop_step()
{
	set_state(State_Step);
}

void AVR_AsyncSimLoop::loop_pause()
{
	set_state(State_Stopped);
}

void AVR_AsyncSimLoop::loop_kill()
{
	set_state(State_Done);
}
