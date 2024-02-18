/*
 * sim_timer.cpp
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

#include "sim_timer.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

PrescaledTimer::PrescaledTimer()
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

PrescaledTimer::~PrescaledTimer()
{
    if (m_parent_timer)
        m_parent_timer->unregister_chained_timer(*this);

    for (PrescaledTimer* timer : m_chained_timers)
        timer->m_parent_timer = nullptr;
}

///Initialise the timer, must be called once during initialisation phases
void PrescaledTimer::init(CycleManager& cycle_manager, Logger& logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = &logger;
}

///Reset the timer. Both stages are reset and disabled.
void PrescaledTimer::reset()
{
    m_ps_max = 1;
    m_ps_factor = 0;
    m_ps_counter = 0;
    m_paused = false;
    m_delay = 0;
    if (!m_updating)
        m_cycle_manager->cancel(*this);
}

/**
   Configure the prescaler
   \param ps_max Maximum value of the prescaler counter, making the prescaler counter wrap to 0
   \param ps_factor Prescaler factor to generate ticks.
          if = 0, the prescaler and timer stages are disabled and reset.
 */
void PrescaledTimer::set_prescaler(unsigned long ps_max, unsigned long ps_factor)
{
    if (!m_updating) update();

    if (!ps_max) ps_max = 1;
    m_ps_max = ps_max;
    m_ps_factor = ps_factor;

    if (!ps_factor)
        m_ps_counter = 0;
    else
        m_ps_counter %= ps_max;

    if (!m_updating) reschedule();
}

/**
   Pause or resume the timer.
   If paused, the prescaler and timer stages are frozen but not reset.
   \param paused true for pausing, false for resuming
 */
void PrescaledTimer::set_paused(bool paused)
{
    if (!m_updating) update();
    m_paused = paused;
    if (!m_updating) reschedule();
}

/**
   Sets the timeout delay to generate a event
   \param delay Timeout delay in prescaler ticks.
          If = 0, the timer stage is disabled and reset.
   \note The prescaler stage is not affected by this setting.
 */
void PrescaledTimer::set_timer_delay(cycle_count_t delay)
{
    if (!m_updating) update();
    m_delay = delay;
    if (!m_updating) reschedule();
}

/**
   Reschedule the timer. This is normally automatically called after
   a configuration change.
 */
void PrescaledTimer::reschedule()
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
        m_cycle_manager->schedule(*this, when);
    else if (scheduled())
        m_cycle_manager->cancel(*this);
}


/**
   Update the timer to catchup with the last completed cycle.
   Ticks may be generated and the signal may be raised if enough cycles have passed.

   If the timer is a child of another timer, the update call is passed on to the parent.
 */
void PrescaledTimer::update()
{
    if (m_cycle_manager->cycle())
        update(m_cycle_manager->cycle() - 1);
}


void PrescaledTimer::update(cycle_count_t when)
{
    if (m_updating) return;
    m_updating = true;

    if (m_parent_timer)
        m_parent_timer->update(when);
    else
        update_timer(when);

    m_updating = false;
}

void PrescaledTimer::update_timer(cycle_count_t when)
{
    //Number of clock cycles since the last update
    cycle_count_t dt = when - m_update_cycle;

    //Checking for dt != 0 ensures ticks are generated only once
    if (dt > 0) {
        process_cycles(dt);
        //Remember the update cycle number for the next update
        m_update_cycle = when;
    }
}

void PrescaledTimer::process_cycles(cycle_count_t cycles)
{
    if (m_ps_factor && !m_paused)
        m_logger->dbg("Prescaled timer processing cycles dt=%lld", cycles);

    //This part generates the prescaler ticks corresponding to the update interval
    //If ticks occurred in the interval, we check if there's enough to trigger the timeout
    //If so, we raise the signal so that the peripheral handles the event.
    //(the peripheral may change the timeout delay and prescaler settings on the fly)
    //We loop by consuming the clock cycles we're catching up with and which generated the ticks
    //We exit the loop once there are not enough clock cycles to generate any tick, or we've
    //been disabled in the signal hook
    while (m_ps_factor && !m_paused) {
        //Calculate the nb of prescaler ticks that occurred in the update interval
        cycle_count_t ticks = (cycles + m_ps_counter % m_ps_factor) / m_ps_factor;

        //Process the chained timers
        for (PrescaledTimer* timer : m_chained_timers)
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

        //Raise the signal to inform the parent peripheral of ticks to consume
        //Decrement the delay by the number of ticks
        signal_data_t sigdata = { .sigid = 0 };
        if (timeout) {
            m_logger->dbg("Prescaled timer timeout, delay=%lld", m_delay);
            sigdata.index = 1;
            sigdata.data = m_delay;
            m_delay = 0;
        } else {
            m_delay -= ticks;
            m_logger->dbg("Prescaled timer generating %lld ticks, remaining=%lld", ticks, m_delay);
            sigdata.index = 0;
            sigdata.data = ticks;
        }
        m_signal.raise(sigdata);

    }
}

/*
 * Methods calculating when the cycle timer should timeout.
 * This is calculated by returning the smallest number of cycles
 * required to reach the timer delay (if non-zero and factoring for
 * the prescaler) of this timer or any of the chained timers.
 * If the timer has a parent, the parent prescaler is factored in.
 */
cycle_count_t PrescaledTimer::calculate_when(cycle_count_t when)
{
    cycle_count_t delay = calculate_delay();
    return (delay > 0) ? (when + delay) : 0;
}

cycle_count_t PrescaledTimer::calculate_delay()
{
    cycle_count_t cycles = 0;

    if (!m_paused && m_ps_factor) {
        if (m_delay)
            cycles = convert_ticks_to_cycles(m_delay);

        for (PrescaledTimer* timer : m_chained_timers) {
            cycle_count_t c = timer->calculate_delay();
            if (c > 0 && (cycles == 0 || c < cycles))
                cycles = c;
        }
    }

    return cycles;
}

cycle_count_t PrescaledTimer::convert_ticks_to_cycles(cycle_count_t ticks)
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
cycle_count_t PrescaledTimer::next(cycle_count_t when)
{
    update(when);
    return calculate_when(when);
}

/**
   Add a timer in the chain.
   \param timer will be added as a child to this timer.
 */
void PrescaledTimer::register_chained_timer(PrescaledTimer& timer)
{
    if (!m_updating) update();

    timer.m_parent_timer = this;
    m_chained_timers.push_back(&timer);

    if (!m_updating) reschedule();
}

/**
   Remove a timer from the chain.
 */
void PrescaledTimer::unregister_chained_timer(PrescaledTimer& timer)
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

/**
   Static helper to compute a timer delay for a particular counter to reach a value.
 */
cycle_count_t PrescaledTimer::ticks_to_event(cycle_count_t counter, cycle_count_t event, cycle_count_t wrap)
{
    cycle_count_t ticks = event - counter + 1;
    if (ticks <= 0)
        ticks += wrap;
    return ticks;
}


//=======================================================================================

/*
 * Implementation of a SignalHook for external clocking. It just forwards to the main class.
 */
class TimerCounter::TimerHook : public SignalHook {

public:

    TimerHook(TimerCounter& timer) : m_timer(timer) {}

    virtual void raised(const signal_data_t& sigdata, int) override
    {
        m_timer.timer_raised(sigdata);
    }

private:

    TimerCounter& m_timer;

};


/*
 * Implementation of a SignalHook for external clocking. It just forwards to the main class.
 */
class TimerCounter::ExtTickHook : public SignalHook {

public:

    ExtTickHook(TimerCounter& timer) : m_timer(timer) {}

    virtual void raised(const signal_data_t&, int) override
    {
        if (m_timer.m_source == Tick_External)
            m_timer.add_tick();
    }

private:

    TimerCounter& m_timer;

};


/**
   Constructor
   \param wrap Wrapping value for the counter. For example, a 16-bits counter wrap is 0x10000.
   \param comp_count number of compare channels
 */
TimerCounter::TimerCounter(long wrap, size_t comp_count)
:m_wrap(wrap)
,m_counter(0)
,m_top(wrap - 1)
,m_source(Tick_Stopped)
,m_slope(Slope_Up)
,m_countdown(false)
,m_cmp(comp_count)
,m_next_event_type(0)
,m_logger(nullptr)
{
    m_timer_hook = new TimerHook(*this);
    m_ext_hook = new ExtTickHook(*this);

    m_timer.signal().connect(*m_timer_hook);
}


TimerCounter::~TimerCounter()
{
    delete m_timer_hook;
    delete m_ext_hook;
}


/**
   Initialise the counter
 */
void TimerCounter::init(CycleManager& cycle_manager, Logger& logger)
{
    m_timer.init(cycle_manager, logger);
    m_logger = &logger;
}


/**
   Reset the counter
 */
void TimerCounter::reset()
{
    m_timer.reset();
    m_source = Tick_Stopped;
    m_slope = Slope_Up;
    m_counter = 0;
    m_countdown = false;
    m_top = m_wrap - 1;

    for (auto& comp : m_cmp) {
        comp.value = 0;
        comp.enabled = false;
    }
}


/**
   Reschedule the counter, this should be called after changing the configuration
 */
void TimerCounter::reschedule()
{
    if (m_source == Tick_Timer)
        m_timer.set_timer_delay(delay_to_event());
    else
        m_timer.set_timer_delay(0);
}


/**
   Change the tick source
 */
void TimerCounter::set_tick_source(TickSource src)
{
    m_source = src;
}


/**
   Progress the counter by one unit. Has no effect if if the source is set to Stopped.
 */
void TimerCounter::tick()
{
    if (m_source != Tick_Stopped)
        add_tick();
}


/**
   Change the TOP value
 */
void TimerCounter::set_top(long top)
{
    m_top = top;
}


/**
   Change the slope mode
 */
void TimerCounter::set_slope_mode(SlopeMode mode)
{
    m_slope = mode;

    if (mode == Slope_Up)
        m_countdown = false;
    else if (mode == Slope_Down)
        m_countdown = true;
}


/**
   Change the counter current value
 */
void TimerCounter::set_counter(long value)
{
    m_counter = value;
}


/**
   Change a compare channel value
 */
void TimerCounter::set_comp_value(size_t index, long value)
{
    m_cmp[index].value = value;
}


/**
   Enable or disable a compare channel
 */
void TimerCounter::set_comp_enabled(size_t index, bool enable)
{
    m_cmp[index].enabled = enable;
}


/**
   Set the counting up or down. Changes the slope mode accordingly except if the slope
   mode is Dual.
 */
void TimerCounter::set_countdown(bool down)
{
    m_countdown = down;

    if (down && m_slope == Slope_Up)
        m_slope = Slope_Down;
    else if (!down && m_slope == Slope_Down)
        m_slope = Slope_Up;
}


long TimerCounter::ticks_to_event(long event)
{
    if (m_countdown)
        return PrescaledTimer::ticks_to_event(event, m_counter, m_wrap);
    else
        return PrescaledTimer::ticks_to_event(m_counter, event, m_wrap);
}


/*
   Calculates the delay in prescaler ticks and the type of the next timer/counter event
   1st step : calculate the delays in ticks to each possible event, determine the
              smallest of them and store it in 'ticks_to_next_event' to be the returned value.
   2st step : store in 'm_next_event_type' the combination of flags TimerEventType corresponding
              to the event, or combination thereof, reached at 'ticks_to_next_event'.
 */
long TimerCounter::delay_to_event()
{
    //Ticks count to reach MAX, i.e. the max value of the counter.
    //Only relevant if counting up
    long ticks_to_max = m_countdown ? m_wrap : ticks_to_event(m_wrap - 1);
    long ticks_to_next_event = ticks_to_max;

    //List of ticks counts to each Output Compare unit
    std::vector<long> comp_ticks = std::vector<long>(m_cmp.size());
    for (size_t i = 0; i < m_cmp.size(); ++i) {
        if (m_cmp[i].enabled) {
            long t = ticks_to_event(m_cmp[i].value);
            comp_ticks[i] = t;
            if (t < ticks_to_next_event)
                ticks_to_next_event = t;
        }
    }

    //Ticks for the counter to reach TOP.
    long ticks_to_top = ticks_to_event(m_top);
    if (ticks_to_top < ticks_to_next_event)
        ticks_to_next_event = ticks_to_top;

    //Ticks count to the bottom value
    long ticks_to_bottom = ticks_to_event(0);
    if (ticks_to_bottom < ticks_to_next_event)
        ticks_to_next_event = ticks_to_bottom;

    //Compile the flag for the next event
    m_next_event_type = 0;
    if (ticks_to_next_event == ticks_to_max)
        m_next_event_type |= Event_Max;
    if (ticks_to_next_event == ticks_to_top)
        m_next_event_type |= Event_Top;
    if (ticks_to_next_event == ticks_to_bottom)
        m_next_event_type |= Event_Bottom;

    for (size_t i = 0; i < m_cmp.size(); ++i) {
        bool is_next_event = (m_cmp[i].enabled && ticks_to_next_event == comp_ticks[i]);
        m_cmp[i].is_next_event = is_next_event;
        if (is_next_event)
            m_next_event_type |= Event_Compare;
    }

    if (m_logger)
        m_logger->dbg("Next event: 0x%02x in %ld cycles", m_next_event_type, ticks_to_next_event);

    return ticks_to_next_event;
}


/*
   Callback from the internal prescaled timer
   Process the timer ticks, by updating the counter
 */
void TimerCounter::timer_raised(const signal_data_t& sigdata)
{
    if (m_logger)
        m_logger->dbg("Updating counters");

    process_ticks(sigdata.data.as_uint(), sigdata.index);
}


/*
 * Callback for a single external clock tick.
 */
void TimerCounter::extclock_raised()
{
    if (m_source == Tick_External)
        tick();
}


/*
 * Single manual clock tick.
 * Determine the events that should be raised for the current value of
 * the counter, then process one tick.
 */
void TimerCounter::add_tick()
{
    m_next_event_type = 0;
    if (m_counter == m_wrap - 1)
        m_next_event_type |= Event_Max;

    for (auto& comp : m_cmp) {
        bool is_next_event = (comp.enabled && m_counter == comp.value);
        comp.is_next_event = is_next_event;
        if (is_next_event)
            m_next_event_type |= Event_Compare;
    }

    if (m_counter == m_top)
        m_next_event_type |= Event_Top;

    if (m_counter == 0)
        m_next_event_type |= Event_Bottom;

    process_ticks(1, m_next_event_type != 0);
}


/*
   Processes the timer clock ticks to update the counter and raise
   the signals for any event reached.
 */
void TimerCounter::process_ticks(long ticks, bool event_reached)
{
    //Update the counter according to the direction
    if (m_countdown)
        m_counter -= ticks;
    else
        m_counter += ticks;

    //If the next event is not reached yet, nothing to process further
    if (!event_reached) return;

    //If the MAX value has been reached
    if (m_next_event_type & Event_Max) {
        if (m_logger)
            m_logger->dbg("Counter reaching MAX value");
        //Edge case to ensure counter wrapping.
        if (!(m_next_event_type & Event_Top))
            m_counter = 0;
    }

    //If one of the Output Compare Unit has been reached
    if (m_next_event_type & Event_Compare) {
        for (size_t i = 0; i < m_cmp.size(); ++i) {
            if (m_cmp[i].is_next_event) {
                if (m_logger)
                    m_logger->dbg("Triggering Compare Match %u" , i);
                m_signal.raise(Signal_CompMatch, m_next_event_type, i);
            }
        }
    }

    //If the TOP value has been reached
    if (m_next_event_type & Event_Top) {
        if (m_logger)
            m_logger->dbg("Counter reaching TOP value");

        //If still counting up, wrap the counter
        //Annoying Special Case : TOP == BOTTOM, the counter must remain at 0
        //and whether the counting is up or down is meaningless
        if (m_slope == Slope_Up || !m_top) {
            m_counter = 0;
        }
        //If double-slopping, change the counting direction
        else if (m_slope == Slope_Double && !m_countdown) {
            m_countdown = true;
            //Here, counter is equal to TOP + 1 but it should be TOP - 1
            m_counter = m_top - 1;
        }
    }

    //If the BOTTOM value has been reached
    if (m_next_event_type & Event_Bottom) {
        if (m_logger)
            m_logger->dbg("Counter reaching BOTTOM value");

        //Check to avoid treating the Annoying Special Case twice
        if (m_top) {
            //If still counting down, wrap the counter
            if (m_slope == Slope_Down) {
                m_counter = m_top;
            }
            //If double-slopping, change the counting direction
            else if (m_slope == Slope_Double && m_countdown) {
                m_countdown = false;
                //Here, counter is equal to BOTTOM - 1 but it should be BOTTOM + 1
                m_counter = 1;
            }
        }
    }

    if (m_logger)
        m_logger->dbg("Counter value: %ld", m_counter);

    m_signal.raise(Signal_Event, m_next_event_type);

    //Set the timer to the next event
    if (m_source == Tick_Timer)
        m_timer.set_timer_delay(delay_to_event());
}
