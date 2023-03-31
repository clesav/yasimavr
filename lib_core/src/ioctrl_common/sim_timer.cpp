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

void AVR_PrescaledTimer::init(AVR_CycleManager& cycle_manager, AVR_Logger& logger)
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
        m_cycle_manager->cancel(*this);
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
        m_cycle_manager->schedule(*this, when);
    else if (scheduled())
        m_cycle_manager->cancel(*this);
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

        m_logger->dbg("Prescaled timer generating %lld ticks, delay=%u", ticks, m_delay);

        //Raise the signal to inform the parent peripheral of ticks to consume
        //Decrement the delay by the number of ticks
        signal_data_t sigdata = { .sigid = 0 };
        if (timeout) {
            m_logger->dbg("Prescaled timer generating %lld ticks, delay=%u", ticks, m_delay);
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

int AVR_PrescaledTimer::ticks_to_event(int counter, int event, int wrap)
{
    int ticks = event - counter + 1;
    if (ticks <= 0)
        ticks += wrap;
    return ticks;
}


//=======================================================================================

/*
 * Implementation of a SignalHook for external clocking. It just forwards to the main class.
 */
class AVR_TimerCounter::TimerHook : public AVR_SignalHook {

public:

    TimerHook(AVR_TimerCounter& timer) : m_timer(timer) {}

    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override
    {
        m_timer.timer_raised(sigdata);
    }

private:

    AVR_TimerCounter& m_timer;

};


/*
 * Implementation of a SignalHook for external clocking. It just forwards to the main class.
 */
class AVR_TimerCounter::ExtTickHook : public AVR_SignalHook {

public:

    ExtTickHook(AVR_TimerCounter& timer) : m_timer(timer) {}

    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override
    {
        m_timer.extclock_raised();
    }

private:

    AVR_TimerCounter& m_timer;

};


AVR_TimerCounter::AVR_TimerCounter(AVR_PrescaledTimer& timer, long wrap, uint32_t comp_count)
:m_wrap(wrap)
,m_counter(0)
,m_top(wrap - 1)
,m_source(Tick_Stopped)
,m_slope(Slope_Up)
,m_countdown(false)
,m_cmp(comp_count)
,m_timer(timer)
,m_next_event_type(0)
,m_logger(nullptr)
{
    m_timer_hook = new TimerHook(*this);
    m_ext_hook = new ExtTickHook(*this);

    m_timer.signal().connect_hook(m_timer_hook);
}


AVR_TimerCounter::~AVR_TimerCounter()
{
    delete m_timer_hook;
    delete m_ext_hook;
}


void AVR_TimerCounter::set_logger(AVR_Logger* logger)
{
    m_logger = logger;
}


void AVR_TimerCounter::reset()
{
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


void AVR_TimerCounter::reschedule()
{
    if (m_source == Tick_Timer)
        m_timer.set_timer_delay(delay_to_event());
    else
        m_timer.set_timer_delay(0);
}


void AVR_TimerCounter::set_tick_source(TickSource src)
{
    m_source = src;
}


void AVR_TimerCounter::set_top(long top)
{
    m_top = top;
}


void AVR_TimerCounter::set_slope_mode(SlopeMode mode)
{
    m_slope = mode;

    if (mode == Slope_Up)
        m_countdown = false;
    else if (mode == Slope_Down)
        m_countdown = true;
}


void AVR_TimerCounter::set_counter(long value)
{
    m_counter = value;
}


void AVR_TimerCounter::set_comp_value(uint32_t index, long value)
{
    m_cmp[index].value = value;
}

void AVR_TimerCounter::set_comp_enabled(uint32_t index, bool enable)
{
    m_cmp[index].enabled = enable;
}


long AVR_TimerCounter::ticks_to_event(long event)
{
    if (m_countdown)
        return AVR_PrescaledTimer::ticks_to_event(event, m_counter, m_wrap);
    else
        return AVR_PrescaledTimer::ticks_to_event(m_counter, event, m_wrap);
}


/*
 * Calculates the delay in prescaler ticks and the type of the next timer/counter event
 * 1st step : calculate the delays in ticks to each possible event, determine the
 *            smallest of them and store it in 'ticks_to_next_event' to be the returned value.
 * 2st step : store in 'm_next_event_type' the combination of flags TimerEventType corresponding
 *            to the event, or combination thereof, reached at 'ticks_to_next_event'.
 */
long AVR_TimerCounter::delay_to_event()
{
    //Ticks count to reach MAX, i.e. the max value of the counter.
    //Only relevant if counting up
    long ticks_to_max = m_countdown ? m_wrap : ticks_to_event(m_wrap - 1);
    long ticks_to_next_event = ticks_to_max;

    //List of ticks counts to each Output Compare unit
    std::vector<long> comp_ticks = std::vector<long>(m_cmp.size());
    for (uint32_t i = 0; i < m_cmp.size(); ++i) {
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

    for (uint32_t i = 0; i < m_cmp.size(); ++i) {
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
 * Callback from the internal prescaled timer
 * Process the timer ticks, by updating the counter
 */
void AVR_TimerCounter::timer_raised(const signal_data_t& sigdata)
{
    if (m_logger)
        m_logger->dbg("Updating counters");

    process_ticks(sigdata.data.as_uint(), sigdata.index);
}


/*
 * Callback for a single external clock tick.
 * Determine the events that should be raised for the current value of
 * the counter, then process one tick.
 */
void AVR_TimerCounter::extclock_raised()
{
    if (m_source != Tick_External) return;

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
 * Processes the timer clock ticks to update the counter and raise
 * the signals for any event reached.
 */
void AVR_TimerCounter::process_ticks(long ticks, bool event_reached)
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
        for (uint32_t i = 0; i < m_cmp.size(); ++i) {
            if (m_cmp[i].is_next_event) {
                if (m_logger)
                    m_logger->dbg("Triggering Compare Match %u" , i);
                m_signal.raise_u(Signal_CompMatch, m_next_event_type, i);
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

    m_signal.raise_u(Signal_Event, m_next_event_type);

    //Set the timer to the next event
    if (m_source == Tick_Timer)
        m_timer.set_timer_delay(delay_to_event());
}
