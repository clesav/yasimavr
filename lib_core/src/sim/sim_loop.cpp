/*
 * sim_loop.cpp
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

#include "sim_loop.h"
#include "../core/sim_debug.h"
#include <thread>
#include <chrono>
#include <climits>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define MIN_SLEEP_THRESHOLD     200

typedef std::chrono::time_point<std::chrono::steady_clock> time_point;

static long long get_timestamp_usecs(time_point origin)
{
    const time_point stamp = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(stamp - origin).count();
}


//=======================================================================================

AbstractSimLoop::AbstractSimLoop(Device& device)
:m_device(device)
,m_state(State_Running)
,m_logger(chr_to_id('S', 'M', 'L', 'P'))
{
    m_logger.set_parent(&m_device.logger());
    m_device.init(m_cycle_manager);
}

cycle_count_t AbstractSimLoop::run_device(cycle_count_t final_cycle)
{
    cycle_count_t cycle_delta = m_device.exec_cycle();

    m_cycle_manager.process_timers();

    Device::State dev_state = m_device.state();

    if (dev_state == Device::State_Sleeping) {

        cycle_count_t next_timer_cycle = m_cycle_manager.next_when();

        if (next_timer_cycle == INVALID_CYCLE) {
            //If the device is sleeping and nothing is scheduled to wake it up,
            //the loop enters standby mode.
            m_state = State_Standby;
            cycle_delta = 1;
            logger().wng("Nothing scheduled yet to wake-up the device, going in standby.");
        }
        else if (next_timer_cycle > final_cycle) {
            m_state = State_Stopped;
            logger().wng("Nothing to process further, stopping.");
        }

        else if (next_timer_cycle > m_cycle_manager.cycle())
            cycle_delta = next_timer_cycle - m_cycle_manager.cycle();

    }

    else if (dev_state >= Device::State_Done) {
        m_state = State_Done;
        logger().wng("Device is done, stopping definitely");
    }

    else if (dev_state == Device::State_Break) {
        m_state = State_Stopped;
        logger().wng("Device hit a break, stopping");
    }

    return cycle_delta;
}


//=======================================================================================

SimLoop::SimLoop(Device& device)
:AbstractSimLoop(device)
,m_fast_mode(false)
{}


/**
   Runs the simulation for a given number of cycles. If set to zero, the simulation
   will run indefinitely and the function will only return when the device stops
   definitely.
 */
void SimLoop::run(cycle_count_t nbcycles)
{
    if (m_state == State_Done) return;

    if (!m_fast_mode && device().frequency() == 0) {
        logger().err("Cannot run in realtime mode, MCU frequency not set.");
        m_state = State_Done;
        return;
    }

    if (m_device.state() < Device::State_Running) {
        logger().err("Device not initialised or firmware not loaded");
        m_state = State_Done;
        return;
    }

    m_state = State_Running;

    const time_point clock_start = std::chrono::steady_clock::now();
    cycle_count_t first_cycle = m_cycle_manager.cycle();
    cycle_count_t final_cycle = (nbcycles > 0) ? (first_cycle + nbcycles - 1) : LLONG_MAX;

    while (m_cycle_manager.cycle() <= final_cycle) {

        cycle_count_t cycle_delta = run_device(final_cycle);

        if (m_state > State_Running)
            break;

        if (!m_fast_mode) {
            //If not in realtime mode, check the simulated clock (given by number of cycles
            //since the start of the loop divided by the clock frequency). It's then compared
            //to the system clock elapsed and if it's ahead by more than a threshold,
            //pause the loop for the time delta
            long long sim_deadline_us = ((m_cycle_manager.cycle() + cycle_delta - first_cycle) *
                                          1000000L) / m_device.frequency();
            long long curr_time_us = get_timestamp_usecs(clock_start);
            long long sleep_time_us = sim_deadline_us - curr_time_us;
            if (sleep_time_us > MIN_SLEEP_THRESHOLD) {
                //WARNING_LOG(m_device.logger(), "LOOP : Sleeping %dus", sleep_time_us);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_us));
            }
        }

        m_cycle_manager.increment_cycle(cycle_delta);

    }

    if (m_state < State_Done) {
        m_state = State_Stopped;
        if (nbcycles > 0)
            m_cycle_manager.increment_cycle(final_cycle - m_cycle_manager.cycle() + 1);
    }

}


//=======================================================================================

AsyncSimLoop::AsyncSimLoop(Device& device)
:AbstractSimLoop(device)
,m_cycling_enabled(false)
,m_cycle_wait(false)
,m_fast_mode(false)
{}

/// Set the simulation running mode: false=real-time, true=fast
void AsyncSimLoop::set_fast_mode(bool fast)
{
    m_fast_mode = fast;
}

/**
   Runs the simulation loop indefinitely. It returns when the loop is killed
   using loop_kill() or the device has stopped definitively.
   The simulation will start in the Stopped state so loop_continue() must be called.
 */
void AsyncSimLoop::run()
{
    if (m_state == State_Done) return;

    if (!m_fast_mode && device().frequency() == 0) {
        logger().err("Cannot run in realtime mode, MCU frequency not set.");
        m_state = State_Done;
        return;
    }

    if (m_device.state() < Device::State_Running) {
        logger().err("Device not initialised or firmware not loaded");
        m_state = State_Done;
        return;
    }

    //Start the loop in stopped state
    set_state(State_Stopped);

    //Mutex used for the condition variables
    std::unique_lock<std::mutex> cv_lock(m_cycle_mutex);
    cv_lock.unlock();

    //Time baseline. Note that it's initialised by the synchronisation part
    cycle_count_t cycle_start = 0;
    std::chrono::time_point<std::chrono::steady_clock> clock_start;

    while (true) {

        //Synchronisation part
        //If m_cycling_enabled is cleared (by start_transaction) or
        //the simulation is stopped, enter a wait loop
        cv_lock.lock();

        while (!m_cycling_enabled || m_state == State_Standby || m_state == State_Stopped) {
            m_cycle_wait = true;
            m_sync_cv.notify_all();
            m_cycle_cv.wait(cv_lock);
            m_cycle_wait = false;

            //The time base is messed up by the pause so re-baseline it
            cycle_start = m_cycle_manager.cycle();
            clock_start = std::chrono::steady_clock::now();
        }

        cv_lock.unlock();

        //If the device can actually run
        if (m_state == State_Running || m_state == State_Step) {

            //Run the device for one instruction and obtain the number of cycles
            //it lasted
            cycle_count_t cycle_delta = run_device(LLONG_MAX);

            if (m_state == State_Step) {
                set_state(State_Stopped);
            }

            else if (m_state == State_Running && !m_fast_mode) {
                //If not in realtime mode, check the simulated clock (given by number of cycles
                //since the start of the loop divided by the clock frequency). It's then compared
                //to the system clock elapsed and if it's ahead by more than a threshold,
                //pause the loop for a catch-up sleep
                long long sim_deadline_us = ((m_cycle_manager.cycle() + cycle_delta - cycle_start)
                                              * 1000000L) / m_device.frequency();
                long long curr_time_us = get_timestamp_usecs(clock_start);
                long long sleep_time_us = sim_deadline_us - curr_time_us;
                if (sleep_time_us > MIN_SLEEP_THRESHOLD) {
                    logger().dbg("Sleeping %lld us", sleep_time_us);

                    //usleep is not used here but rather cond_var.wait_for() so that
                    //a transaction may interrupt a catch-up sleep

                    cv_lock.lock();
                    std::chrono::microseconds t_us(sleep_time_us);

                    if (m_cycle_cv.wait_for(cv_lock, t_us) == std::cv_status::no_timeout) {
                        //Arriving here means the catch-up sleep has been interrupted

                        //Try to estimate which cycle number we end up in.
                        //It's calculated by looking at the total time spent since the start and
                        //deriving a corresponding cycle number. The *real* cycle delta is then the
                        //difference between this estimated cycle number and the cycle number we were
                        //at the start of the sleep (it is the current value stored in the cycle manager)
                        time_point clock_t1 = std::chrono::steady_clock::now();
                        int64_t time_delta_us = std::chrono::duration_cast<std::chrono::microseconds>(clock_t1 - clock_start).count();
                        cycle_count_t cycle_corrected = cycle_start + time_delta_us * m_device.frequency() / 1000000L;
                        cycle_count_t cycle_delta_corrected = cycle_corrected - m_cycle_manager.cycle();

                        //Constrain the corrected value to ensure the delta is at least 1 and at most
                        //the initial delta minus 1
                        //The "at least 1" is because cycle numbers should always increment
                        //The "minus 1" is because it gives a chance for whatever signal interrupted
                        //the sleep to trigger changes before whatever was scheduled to happen at the
                        //end of the sleep actually happen.
                        if (cycle_delta_corrected < 1)
                            cycle_delta_corrected = 1;
                        if (cycle_delta_corrected >= cycle_delta)
                            cycle_delta_corrected = cycle_delta - 1;

                        cycle_delta = cycle_delta_corrected;
                    }

                    cv_lock.unlock();
                }
            }

            m_cycle_manager.increment_cycle(cycle_delta);

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

/**
   Start a transaction, which designates any interaction with any
   interface of the simulated device.
   This will pause the simulation between cycles, to ensure the consistency
   of the device model data.
 */
bool AsyncSimLoop::start_transaction()
{
    std::unique_lock<std::mutex> lock(m_cycle_mutex);

    //In case the transaction allows the device to wake-up from
    //unlimited sleep, or at least schedule timer, wake up the loop.
    //If nothing is scheduled, the loop will go back in standby at the next
    //cycle
    if (m_state == State_Standby)
        m_state = State_Running;

    //Setting this flag will block the simloop
    m_cycling_enabled = false;

    //This notify ensures the simloop wakes up from a catchup sleep
    m_cycle_cv.notify_all();

    //Wait until the simloop reaches the synchronisation part or leaves the loop
    while (!m_cycle_wait && m_state != State_Done)
        m_sync_cv.wait(lock);

    //Returns a indication whether the loop left or keeps running
    return m_state != State_Done;
}

/**
   End a transaction and let the simulation resume.
 */
void AsyncSimLoop::end_transaction()
{
    std::unique_lock<std::mutex> lock(m_cycle_mutex);

    //Reset the simloop block and notify it
    m_cycling_enabled = true;
    m_cycle_cv.notify_all();
}

/**
   Resumes the loop when it's in the Stopped state.
   The loop is initialised in the Stopped state so this must be called
   at the start of a simulation.
   Must be surrounded by start_transaction() / end_transaction()
 */
void AsyncSimLoop::loop_continue()
{
    if (m_state < State_Done)
        set_state(State_Running);
}

/**
   Instruct the device model to execute one instruction and stop.
   Must be surrounded by start_transaction() / end_transaction()
 */
void AsyncSimLoop::loop_step()
{
    if (m_state == State_Stopped)
        set_state(State_Step);
}

/**
   Stop the simulation.
   Must be surrounded by start_transaction() / end_transaction()
 */
void AsyncSimLoop::loop_pause()
{
    if (m_state < State_Done)
        set_state(State_Stopped);
}

/**
   Stop definitively the simulation. run() will exit.
   Must be surrounded by start_transaction() / end_transaction()
 */
void AsyncSimLoop::loop_kill()
{
    set_state(State_Done);
}
