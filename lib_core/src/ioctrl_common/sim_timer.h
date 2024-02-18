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

/// \defgroup api_timer Timer/Counter framework

/**
   \ingroup api_timer
   \brief Generic model of a Timer with prescaling.

   Implementation of a clock cycle timer, used by peripherals such as TCx, WDT, RTC.

   It is structured with two consecutive stages:
    - Prescaler
    - Timer

   The prescaler works as a counter of simulated clock cycles, starting at 0,
   wrapping at 'ps_max', and generating timer 'ticks' once every 'ps_factor' cycles.
   The timer generates a timeout signal after a delay given in prescaler ticks.

   The timeout is transmitted though a Signal, available via signal() and raised in 2 ways:
    - When the programmed timeout delay is reached.
    - When update() is called, and enough clock cycles have passed, resulting in at least one tick.

   If, during the update, the number of generated ticks is enough to reach the timer delay,
   the signal index is set to 1, otherwise it is set to 0. The signal data field is set to the
   generated tick count.

   Timers can be daisy-chained, so that the prescaler tick output of a timer feeds into the
   prescaler clock input of another.
 */
class AVR_CORE_PUBLIC_API PrescaledTimer : public CycleTimer {

public:

    PrescaledTimer();
    virtual ~PrescaledTimer();

    void init(CycleManager& cycle_manager, Logger& logger);

    void reset();

    void set_prescaler(unsigned long ps_max, unsigned long ps_factor);

    unsigned long prescaler_factor() const;

    void set_timer_delay(cycle_count_t delay);

    cycle_count_t timer_delay() const;

    void set_paused(bool paused);

    void update();

    virtual cycle_count_t next(cycle_count_t when) override;

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
    Signal m_signal;                    //Signal raised for processing ticks

    //***** Timer chain management *****
    std::vector<PrescaledTimer*> m_chained_timers;
    PrescaledTimer* m_parent_timer;

    void reschedule();
    void update(cycle_count_t when);
    void update_timer(cycle_count_t when);
    void process_cycles(cycle_count_t cycles);

    cycle_count_t calculate_when(cycle_count_t when);
    cycle_count_t calculate_delay();
    cycle_count_t convert_ticks_to_cycles(cycle_count_t ticks);

};

/// Getter for ps_factor.
inline unsigned long PrescaledTimer::prescaler_factor() const
{
    return m_ps_factor;
}

/// Getter for timer_delay.
inline cycle_count_t PrescaledTimer::timer_delay() const
{
    return m_delay;
}

/// Getter for the signal raised with counter updates
inline Signal& PrescaledTimer::signal()
{
    return m_signal;
}


//=======================================================================================
/**
   \ingroup api_timer
   \brief Generic model of a Counter.

   Implementation of a clock cycle counter, used by peripherals such as TCx, WDT, RTC.
   Features :
    - 2 'tick' sources: internal (using a PrescaledTimer object) or external via a signal hook
    - Up/down counting and dual slope
    - Arbitrary number of compare channels
    - Signalling on top, bottom, max and compare value
 */
class AVR_CORE_PUBLIC_API TimerCounter {

public:

    /// Tick source mode
    enum TickSource {
        /// Counter stopped
        Tick_Stopped = 0,
        /// Internal prescaled timer used as tick source
        Tick_Timer,
        /// External signal hook used as tick source
        Tick_External,
    };

    /// Counter direction mode.
    enum SlopeMode {
        /// Up-counting
        Slope_Up = 0,
        /// Down-counting
        Slope_Down,
        /// Dual-slope counting
        Slope_Double,
    };

    /// Event type flags used when signaling. \sa Signal_Event
    enum EventType {
        /// The counter is wrapping
        Event_Max     = 0x01,
        /// The counter has reached the TOP value
        Event_Top     = 0x02,
        /// The counter has reached the BOTTOM value (zero)
        Event_Bottom  = 0x04,
        /// The counter has reached one of the Compare channel values
        Event_Compare = 0x08
    };

    /// Signal Ids raised by this object.
    enum SignalId {
        /**
           Signal raised on a overflow event, the data is a combination
           of EventType flags, indicating the type(s) of event.
         */
        Signal_Event,
        /**
           Signal raised on a Compare Match event. The index indicates which channel.
           No data is carried.
         */
        Signal_CompMatch,
    };

    TimerCounter(long wrap, size_t comp_count);
    ~TimerCounter();

    void init(CycleManager& cycle_manager, Logger& logger);

    void reset();
    void reschedule();
    void update();

    long wrap() const;

    void set_tick_source(TickSource src);
    TickSource tick_source() const;

    void tick();

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
    void set_countdown(bool down);

    Signal& signal();
    SignalHook& ext_tick_hook();

    PrescaledTimer& prescaler();

    //no copy semantics
    //TimerCounter(const TimerCounter&) = delete;
    //TimerCounter& operator=(const TimerCounter&) = delete;

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
    PrescaledTimer m_timer;
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
    void add_tick();
    long ticks_to_event(long event);
    void process_ticks(long ticks, bool event_reached);

};

/// Getter for the wrapping value
inline long TimerCounter::wrap() const
{
    return m_wrap;
}


/// Force update of the internal prescaler
inline void TimerCounter::update()
{
    m_timer.update();
}


/// Getter for the tick source mode
inline TimerCounter::TickSource TimerCounter::tick_source() const
{
    return m_source;
}

/// Getter for the TOP value
inline long TimerCounter::top() const
{
    return m_top;
}

/// Getter for the slope mode
inline TimerCounter::SlopeMode TimerCounter::slope_mode() const
{
    return m_slope;
}

/// Getter for the current counter value
inline long TimerCounter::counter() const
{
    return m_counter;
}

/// Getter for a compare value
inline long TimerCounter::comp_value(size_t index) const
{
    return m_cmp[index].value;
}

/// Getter for a compare value enable
inline bool TimerCounter::comp_enabled(size_t index) const
{
    return m_cmp[index].enabled;
}

/// Getter for the current counting direction
inline bool TimerCounter::countdown() const
{
    return m_countdown;
}

/// Getter for the counting signal
inline Signal& TimerCounter::signal()
{
    return m_signal;
}

/// Getter for the external signal hook used for tick source
inline SignalHook& TimerCounter::ext_tick_hook()
{
    return *reinterpret_cast<SignalHook*>(m_ext_hook);
}

/// Getter for the internal prescaler
inline PrescaledTimer& TimerCounter::prescaler()
{
    return m_timer;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_IO_TIMER_H__
