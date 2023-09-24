/*
 * sim_loop.h
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

#ifndef __YASIMAVR_LOOP_H__
#define __YASIMAVR_LOOP_H__

#include "../core/sim_types.h"
#include "../core/sim_device.h"
#include <vector>
#include <mutex>
#include <condition_variable>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Common base class for simulation loops.
 */
class AVR_CORE_PUBLIC_API AbstractSimLoop {

public:

    enum State {
        State_Running,
        State_Step,
        State_Standby,
        State_Stopped,
        State_Done
    };

    explicit AbstractSimLoop(Device& device);
    virtual ~AbstractSimLoop() = default;

    AbstractSimLoop::State state() const;
    cycle_count_t cycle() const;
    CycleManager& cycle_manager();
    const Device& device() const;
    Logger& logger();

protected:

    Device& m_device;
    State m_state;
    CycleManager m_cycle_manager;
    Logger m_logger;

    cycle_count_t run_device(cycle_count_t final_cycle);
    void set_state(AbstractSimLoop::State state);

};

inline AbstractSimLoop::State AbstractSimLoop::state() const
{
    return m_state;
}

inline void AbstractSimLoop::set_state(State state)
{
    m_state = state;
}

inline cycle_count_t AbstractSimLoop::cycle() const
{
    return m_cycle_manager.cycle();
}

inline CycleManager& AbstractSimLoop::cycle_manager()
{
    return m_cycle_manager;
}

inline const Device& AbstractSimLoop::device() const
{
    return m_device;
}

inline Logger& AbstractSimLoop::logger()
{
    return m_logger;
}


//=======================================================================================
/**
   \brief Synchronous simulation loop
   Basic synchronous simulation loop. It is designed for "fast" simulations with
   a deterministic set of stimuli.
   It can run in "fast" mode or "real-time" mode
    - In real-time mode : the simulation will try to adjust the speed of the simulation
    to align the simulated time with the system time.
    - In fast mode : no adjustment is done and the simulation runs as fast as permitted.
 */
class AVR_CORE_PUBLIC_API SimLoop : public AbstractSimLoop {

public:

    explicit SimLoop(Device& device);

    void set_fast_mode(bool fast);

    void run(cycle_count_t count = 0);

private:

    bool m_fast_mode;

};

/// Set the simulation running mode: false=real-time, true=fast
inline void SimLoop::set_fast_mode(bool fast)
{
    m_fast_mode = fast;
}

//=======================================================================================
/**
   \brief Asynchronous simulation loop
   It is designed when simulation need to interact with code running in another thread.
   Examples: debugger, GUI, sockets.

   The simulation library in itself is not thread-safe.
   The synchronization is done by using the methods start_transaction and end_transaction
   which *must* surround any call to any interface to the simulated device. The effect
   is to block the simulation loop between cycles so that the state stays consistent throughout
   the simulated MCU.
 */
class AVR_CORE_PUBLIC_API AsyncSimLoop : public AbstractSimLoop {

public:

    explicit AsyncSimLoop(Device& device);
    void set_fast_mode(bool fast);

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
    bool m_fast_mode;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_LOOP_H__
