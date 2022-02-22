/*
 * sim_loop.h
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

#ifndef __YASIMAVR_LOOP_H__
#define __YASIMAVR_LOOP_H__

#include "../core/sim_types.h"
#include "../core/sim_device.h"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>


//=======================================================================================

class DLL_EXPORT AVR_AbstractSimLoop {

public:

    enum State {
		State_Running,
		State_Step,
		State_Stopped,
		State_Done
    };

    class Hook : public AVR_CycleTimer {

    public:

    	Hook(AVR_AbstractSimLoop& simloop);
    	Hook(AVR_AbstractSimLoop& simloop, cycle_count_t when);

    	virtual cycle_count_t next(cycle_count_t when, bool* stop) = 0;
    	virtual cycle_count_t next(cycle_count_t when) override final;

    private:

    	AVR_AbstractSimLoop& m_simloop;

    };

    friend class Hook;

    AVR_AbstractSimLoop(AVR_Device& device);
    ~AVR_AbstractSimLoop();

    AVR_AbstractSimLoop::State state() const;
    cycle_count_t cycle() const;
    AVR_CycleManager& cycle_manager();
	const AVR_Device& device() const;

protected:

	AVR_Device& m_device;
	State m_state;
	AVR_CycleManager m_cycle_manager;

	void run_device(cycle_count_t cycle_limit);
	void set_state(AVR_AbstractSimLoop::State state);

};

inline AVR_AbstractSimLoop::State AVR_AbstractSimLoop::state() const
{
	return m_state;
}

inline void AVR_AbstractSimLoop::set_state(State state)
{
	m_state = state;
}

inline cycle_count_t AVR_AbstractSimLoop::cycle() const
{
	return m_cycle_manager.cycle();
}

inline AVR_CycleManager& AVR_AbstractSimLoop::cycle_manager()
{
	return m_cycle_manager;
}

inline const AVR_Device& AVR_AbstractSimLoop::device() const
{
	return m_device;
}


//=======================================================================================

class DLL_EXPORT AVR_SimLoop : public AVR_AbstractSimLoop {

public:

    AVR_SimLoop(AVR_Device& device);

	void set_fast_mode(bool fast);

	void run(cycle_count_t count = 0);

private:

	bool m_fast_mode;

};


//=======================================================================================

class DLL_EXPORT AVR_AsyncSimLoop : public AVR_AbstractSimLoop {

public:

	AVR_AsyncSimLoop(AVR_Device& device);

	void run();

	bool start_transaction();
	void end_transaction();

	void loop_continue();
	void loop_pause();
	void loop_step();
	void loop_kill();

private:

	std::mutex m_cycle_mutex;
	std::condition_variable m_cycle_cv;
	std::condition_variable m_sync_cv;
	bool m_cycling_enabled;
	bool m_cycle_wait;

};


#endif //__YASIMAVR_LOOP_H__
