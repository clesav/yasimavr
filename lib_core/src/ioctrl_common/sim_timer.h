/*
 * sim_timer.h
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

#ifndef __YASIMAVR_IO_TIMER_H__
#define __YASIMAVR_IO_TIMER_H__

#include "../core/sim_cycle_timer.h"
#include "../core/sim_signal.h"
#include "../core/sim_device.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Generic implementation of a clock cycle timer, used by the peripherals TCx, WDT, RTC
 * It is structured with two consecutive stages:
 *  - Prescaler
 *  - Timer
 *  The prescaler works as a counter of simulated clock cycles, starting at 0,
 *  wrapping at 'ps_max', and generating timer 'ticks' once every 'ps_factor' cycles.
 *  The timer generates a timeout signal after a delay given in prescaler ticks.
 *  The class doesn't not have any tick counter as such. it only generates ticks that user objects
 *  can use to increment or decrement a counter. The meaning of the timeout signal is also left
 *  to users.
 *  The timeout is transmitted though a Signal, available via 'signal()' and raised in 2 ways:
 *   - When the programmed timeout delay is reached
 *   - When 'update()' is called, and enough clock cycles have passed, resulting in at least one tick.
 *  If the nb of ticks is enough to reach the set delay, the signal data index is set to 1.
 *  Otherwise, data.index is set to 0 and data.u is set to the available tick count.
 *  Timers can be daisy-chained, so that the prescaler output of a timer feeds into the
 *  prescaler of another.
 */
class AVR_CORE_PUBLIC_API PrescaledTimer : public CycleTimer {

public:

    PrescaledTimer();
    virtual ~PrescaledTimer();

    //Initialise the timer, must be called once during initialisation phases
    void init(CycleManager& cycle_manager, Logger& logger);
    //Reset the timer. Both stages are reset and disabled
    void reset();
    //Configure the prescaler:
    // - ps_max is the maximum value of the prescaler counter, making
    //   the prescaler counter wrap to 0
    // - ps_factor is the prescaler factor to generate ticks
    //   if ps_factor = 0, the prescaler and timeout delay are disabled and reset
    void set_prescaler(unsigned long ps_max, unsigned long ps_factor);
    //Getter for ps_factor
    unsigned long prescaler_factor() const;
    //Sets the timeout delay to generate a event
    //If delay = 0, the timeout delay is disabled and reset
    //The prescaler is not affected by this setting
    void set_timer_delay(cycle_count_t delay);
    //Getter for timer_delay
    cycle_count_t timer_delay() const;
    //Pauses the timer
    //If paused, the prescaler and timeout delay are frozen but not reset
    void set_paused(bool paused);
    //Update the timer to catchup with the 'when' cycle
    //Ticks may be generated and the signal may be raised if enough cycles have passed
    void update(cycle_count_t when = INVALID_CYCLE);

    //Callback override from CycleTimer
    virtual cycle_count_t next(cycle_count_t when) override;

    //Returns the signal that is raised with ticks
    Signal& signal();

    void register_chained_timer(PrescaledTimer& timer);
    void unregister_chained_timer(PrescaledTimer& timer);

    static cycle_count_t ticks_to_event(cycle_count_t counter, cycle_count_t event, cycle_count_t wrap);

    //Disable copy semantics
    PrescaledTimer(const PrescaledTimer&) = delete;
    PrescaledTimer& operator=(const PrescaledTimer&) = delete;

private:

    CycleManager* m_cycle_manager;
    Logger* m_logger;

    //***** Prescaler management *****
    unsigned long m_ps_max;             //Max value of the prescaler
    unsigned long m_ps_factor;          //Prescaler division factor (Tick period / Clock period)
    cycle_count_t m_ps_counter;         //Prescaler counter

    //***** Delay management *****
    cycle_count_t m_delay;              //Delay until the next timeout in ticks

    //***** Update management *****
    bool m_paused;                      //Boolean indicating if the timer is paused
    bool m_updating;                    //Boolean used to avoid infinite updating reentrance
    cycle_count_t m_update_cycle;       //Cycle number of the last update
    Signal m_signal;                //Signal raised for processing ticks

    //***** Timer chain management *****
    std::vector<PrescaledTimer*> m_chained_timers;
    PrescaledTimer* m_parent_timer;

    void reschedule();
    void update_timer(cycle_count_t when);
    void process_cycles(cycle_count_t cycles);

    cycle_count_t calculate_when(cycle_count_t when);
    cycle_count_t calculate_delay();
    cycle_count_t convert_ticks_to_cycles(cycle_count_t ticks);

};

inline unsigned long PrescaledTimer::prescaler_factor() const
{
    return m_ps_factor;
}

inline cycle_count_t PrescaledTimer::timer_delay() const
{
    return m_delay;
}

inline Signal& PrescaledTimer::signal()
{
    return m_signal;
}


//=======================================================================================
/*
 * Implementation of a generic Timer/Counter for AVR series
 * This timer is a flexible implementation aiming at covering most modes found in
 * AVR timer/counter. It covers normal, CTC, PWM in both single and dual slopes.
 * It has a arbitrary number of Compare Units that raise signals when the counter
 * matches their values.
 * A TimerCounter object must be associated to a PrescaledTimer object. The TimerCounter
 * will set the timer delay but configuring the PrescaledTimer object, in particular theis up
 * to the user.
 */

class AVR_CORE_PUBLIC_API TimerCounter {

public:

    enum TickSource {
        Tick_Stopped = 0,
        Tick_Timer,
        Tick_External,
    };

    enum SlopeMode {
        Slope_Up = 0,
        Slope_Down,
        Slope_Double,
    };

    enum EventType {
        Event_Max     = 0x01,
        Event_Top     = 0x02,
        Event_Bottom  = 0x04,
        Event_Compare = 0x08
    };

    enum SignalId {
        //Signal raised on a overflow event, the data is a uint, combination
        //of EventType flags, indicating the type(s) of event
        Signal_Event,
        //Signal raised on a Compare Match event. The index indicates which channel
        //(0='A', 1='B', etc...), no data is carried.
        Signal_CompMatch,
    };

    TimerCounter(PrescaledTimer& timer, long wrap, size_t comp_count);
    ~TimerCounter();

    long wrap() const;

    void reset();
    void reschedule();

    void set_tick_source(TickSource src);
    TickSource tick_source() const;

    void set_top(long top);
    long top() const;

    void set_slope_mode(SlopeMode mode);
    SlopeMode slope_mode() const;

    void set_counter(long value);
    long counter() const;

    void set_comp_value(size_t index, long value);
    long comp_value(size_t index) const;

    void set_comp_enabled(size_t index, bool enable);
    bool comp_enabled(size_t index) const;

    bool countdown() const;

    Signal& signal();
    SignalHook& ext_tick_hook();

    void set_logger(Logger* logger);

    //no copy semantics
    TimerCounter(const TimerCounter&) = delete;
    TimerCounter& operator=(const TimerCounter&) = delete;

private:

    class TimerHook;
    friend class TimerHook;

    class ExtTickHook;
    friend class ExtTickHook;

    struct CompareUnit {
        long value = 0;
        bool enabled = false;
        bool is_next_event = false;
    };

    //Selected tick source
    TickSource m_source;
    //Counter wrap value
    long m_wrap;
    //Current counter value
    long m_counter;
    //Top value
    long m_top;
    //Slope mode
    SlopeMode m_slope;
    //Indicates if the counter is counting up (false) or down (true)
    bool m_countdown;
    //List of compare units
    std::vector<CompareUnit> m_cmp;
    //Event timer engine
    PrescaledTimer& m_timer;
    //Flag variable storing the next event type(s)
    uint8_t m_next_event_type;
    //Signal management
    DataSignal m_signal;
    TimerHook* m_timer_hook;
    ExtTickHook* m_ext_hook;
    //Logging
    Logger* m_logger;

    long delay_to_event();
    void timer_raised(const signal_data_t& sigdata);
    void extclock_raised();
    long ticks_to_event(long event);
    void process_ticks(long ticks, bool event_reached);

};

inline long TimerCounter::wrap() const
{
    return m_wrap;
}

inline TimerCounter::TickSource TimerCounter::tick_source() const
{
    return m_source;
}

inline long TimerCounter::top() const
{
    return m_top;
}

inline TimerCounter::SlopeMode TimerCounter::slope_mode() const
{
    return m_slope;
}

inline long TimerCounter::counter() const
{
    return m_counter;
}

inline long TimerCounter::comp_value(size_t index) const
{
    return m_cmp[index].value;
}

inline bool TimerCounter::comp_enabled(size_t index) const
{
    return m_cmp[index].enabled;
}

inline bool TimerCounter::countdown() const
{
    return m_countdown;
}

inline Signal& TimerCounter::signal()
{
    return m_signal;
}

inline SignalHook& TimerCounter::ext_tick_hook()
{
    return *reinterpret_cast<SignalHook*>(m_ext_hook);
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_IO_TIMER_H__
