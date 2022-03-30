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


//=======================================================================================
/*
 * Base class for a simloop
 */

class DLL_EXPORT AVR_AbstractSimLoop {

public:

    enum State {
		State_Running,
		State_Step,
		State_Stopped,
		State_Done
    };

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

	cycle_count_t run_device(cycle_count_t cycle_limit);
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
/*
 * AVR_SimLoop is a basic synchronous simulation loop. It is designed for "fast" simulations
 * with a deterministic set of stimuli.
 * It can run in "fast" mode or "real-time" mode
 *  - In real-time mode : the simulation will try to adjust the speed of the simulation
 *  to align the simulated time with the system time.
 *  - In fast mode : no adjustment is done and the simulation runs as fast as permitted.
 */
class DLL_EXPORT AVR_SimLoop : public AVR_AbstractSimLoop {

public:

    AVR_SimLoop(AVR_Device& device);

    //Set the simulation running mode: false=real-time, true=fast
	void set_fast_mode(bool fast);

	//Runs the simulation for a given number of cycles. If set to zero, the simulation
	//will run indefinitely and the function will only return when the device stops
	//definitely
	void run(cycle_count_t count = 0);

private:

	bool m_fast_mode;

};

inline void AVR_SimLoop::set_fast_mode(bool fast)
{
	m_fast_mode = fast;
}

//=======================================================================================
/*
 * AVR_SimLoop is a asynchronous simulation loop. It is designed when simulation need to
 * interact with code running in another thread. Examples: debugger, GUI, sockets.
 * The simulation library in itself is not thread-safe.
 * The synchronization is done by using the methods start_transaction and end_transaction
 * which *must* surround any call to any interface to the simulated device. The effect
 * is to block the simulation loop between cycles so that the state stays consistent throughout
 * the simulated MCU.
 */

class DLL_EXPORT AVR_AsyncSimLoop : public AVR_AbstractSimLoop {

public:

	AVR_AsyncSimLoop(AVR_Device& device);

	//Set the simulation running mode: false=real-time, true=fast
	void set_fast_mode(bool fast);

	//Runs the simulation loop indefinitely. It returns when the loop is killed
	//using (loop_kill) or the device has stopped definitely.
	//The simulation wil start in the Stopped state so loop_continue must be called
	void run();

	//Methods to start/end a transaction, which designate any interaction with any
	//interface of the simulated device.
	bool start_transaction();
	void end_transaction();

	//Utilities to control the execution of the simulation loop. They must be surrounded
	//by start_transaction/end_transaction.
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
	bool m_fast_mode;

};


#endif //__YASIMAVR_LOOP_H__
