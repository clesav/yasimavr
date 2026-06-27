/*
 * sim_cycle_timer.cpp
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

#include "sim_cycle_timer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

// Flags for the m_state variable in CycleTimer
// Indicates that the timer is scheduled with the manager
#define STATE_SCHEDULED  0x01
// Indicates that the timer is 'processing' i.e. we are currently in its 'next' callback
#define STATE_PROCESSING 0x02
// Indicated that the timer is paused
#define STATE_PAUSED     0x04
// Indicates that the timer's clock domain has no frequency, i.e. the source is turned off.
#define STATE_NOCLOCK    0x08
// Indicates that the timer delay had been specified in seconds rather than cycles
#define STATE_SECS       0x10

//Useful combination of the PAUSED and NOCLOCK flag indicating the timer can't be processed
#define STATE_HALTED     (STATE_PAUSED | STATE_NOCLOCK)


CycleTimer::CycleTimer()
:m_manager(nullptr)
,m_state(0)
,m_when_d(0.0)
,m_when(INVALID_CYCLE)
{}


CycleTimer::~CycleTimer()
{
    if (m_state & STATE_SCHEDULED)
        m_manager->cancel(*this);
}


CycleTimer::CycleTimer(const CycleTimer& other)
:CycleTimer()
{
    *this = other;
}


CycleTimer& CycleTimer::operator=(const CycleTimer& other)
{
    if (m_state & STATE_SCHEDULED)
        m_manager->cancel(*this);

    m_manager = other.m_manager;

    if (other.m_manager)
        other.m_manager->copy_slot(other, *this);

    return *this;
}

/**
 * Initialisation of the timer by assigning it a manager and a clock domain.
 * Must be called before any operation on the timer.
 */
void CycleTimer::init(CycleManager& manager, sim_id_t clock_domain)
{
    m_manager = &manager;
    m_clock_domain = clock_domain;
}


[[noreturn]]
static void throw_uninitialised()
{
    throw std::runtime_error("Timer not initialised");
}


/**
   Schedule or reschedule a timer for call in 'delay' cycles
   \param delay delay from the current cycle, in cycle number
 */
void CycleTimer::delay(cycle_count_t count)
{
    if (count <= 0) {
        cancel();
        return;
    }

    if (!m_manager)
        throw_uninitialised();

    m_manager->delay(*this, count);
}

/**
   Schedule or reschedule a timer for call in 'delay' seconds
   \param secs delay from the current cycle, in seconds
 */
void CycleTimer::delay_s(double secs)
{
    if (secs <= 0) {
        cancel();
        return;
    }

    if (!m_manager)
        throw_uninitialised();

    m_manager->delay_s(*this, secs);
}


/**
   Remove this timer from the scheduling queue. No-op if not scheduled.
 */
void CycleTimer::cancel()
{
    if (m_state & STATE_SCHEDULED)
        m_manager->cancel(*this);
}


/**
   Pause this timer.

   The timer stays in the scheduling queue but won't be called until it's resumed.
   The remaining delay until the timer 'when' is conserved during the pause.
   \sa resume
 */
void CycleTimer::pause()
{
    if ((m_state & STATE_SCHEDULED) && !(m_state & STATE_PAUSED))
        m_manager->pause(*this);
}

/**
   Resume a paused timer.
   \sa pause
 */
void CycleTimer::resume()
{
    if (m_state & STATE_PAUSED)
        m_manager->resume(*this);
}

/**
 * Returns true if this timer is scheduled with a manager.
 */
bool CycleTimer::scheduled() const
{
    return m_state & STATE_SCHEDULED;
}

/**
 * Returns true if the timer is "processing" i.e. we are currently
 * in its "next" callback
 */
bool CycleTimer::processing() const
{
    return m_state & STATE_PROCESSING;
}

/**
 * Returns true if the timer is paused.
 */
bool CycleTimer::paused() const
{
    return m_state & STATE_PAUSED;
}

/**
   Returns the remaining delay before this timer is called, or -1 if the timer isn't scheduled.
 */
cycle_count_t CycleTimer::remaining_delay() const
{
    if (!m_state)
        return INVALID_CYCLE;

    if (m_state & STATE_NOCLOCK)
        return m_when;

    cycle_count_t d;
    if (m_state & STATE_PAUSED)
        d = m_when;
    else if (m_when < m_manager->m_cycle)
        d = 0;
    else
        d = m_when - m_manager->m_cycle;

    return std::ceil(d / m_manager->m_clock_domains.at(m_clock_domain).cycle_factor);
}


//=======================================================================================

CycleManager::CycleManager()
:m_cycle(0)
,m_processing(false)
,m_processed_when(0)
,m_post_process_clock_update(false)
,m_ref_clk_freq(1.0)
,m_ref_cycle(0)
,m_ref_time(0.0)
{
    m_clock_sources[sim_id_t()] = clock_source_t{ 1.0 };
    //ensure the reference domain always exists
    m_clock_domains[ReferenceDomain] = clock_domain_t{ sim_id_t(), 1, 1, 1.0 };
}


CycleManager::~CycleManager()
{
    for (auto t : m_timer_slots) {
        t->m_state = 0;
        t->m_manager = nullptr;
    }
}

/**
 * Returns the simulating elapsed time based on the current cycle, taking into account
 * \return elapsed simulating time in seconds
 */
double CycleManager::elapsed_time() const
{
    return m_ref_time + (m_cycle - m_ref_cycle) / m_ref_clk_freq;
}

/**
   Increment the cycle counter.
 */
void CycleManager::increment_cycle(cycle_count_t count)
{
    m_cycle += count;
}


/**
 * Add a new clock source.
 * \param id clock source ID
 */
void CycleManager::add_clock_source(sim_id_t id)
{
    m_clock_sources[id] = { 1.0 };
    update_clocks();
}

/**
 * Configure a clock source by setting its frequency.
 * The source can be turned off by setting the frequency to zero.
 * \param id clock source ID
 * \param frequency new frequency. Can be >= 0
 */
void CycleManager::configure_clock_source(sim_id_t id, double frequency)
{
    if (frequency < 0.0)
        frequency = 0.0;

    m_clock_sources.at(id) = { frequency };

    update_clocks();
}

/**
 * Add a new clock domain
 */
void CycleManager::add_clock_domain(sim_id_t id)
{
    m_clock_domains[id] = clock_domain_t{ sim_id_t(), 1, 1, 1.0 };
    update_clocks();
}

/**
 * Configure a clock domain by selecting a source and setting prescaler factors
 * \param id clock domain ID
 * \param source clock source ID selected for this domain
 * \param ps_div prescaler dividing factor
 * \param ps_mul prescaler multiplying factor
 */
void CycleManager::configure_clock_domain(sim_id_t id, sim_id_t source, unsigned long ps_div, unsigned long ps_mul)
{
    if (!ps_div) ps_div = 1;

    clock_domain_t& clkdom = m_clock_domains.at(id);
    clkdom.src = source;
    clkdom.ps_div = ps_div;
    clkdom.ps_mul = ps_mul;

    update_clocks();
}

/**
 * Returns a clock domain frequency
 */
double CycleManager::domain_frequency(sim_id_t id) const
{
    auto& clkdom = m_clock_domains.at(id);
    auto& clksrc = m_clock_sources.at(clkdom.src);
    if (clksrc.frequency && clkdom.ps_mul)
        return clksrc.frequency * (double)clkdom.ps_mul / (double)clkdom.ps_div;
    else
        return 0.0;
}


void CycleManager::update_clocks()
{
    //If the clock update is done during the processing of cycle timers, we need
    //to wait until all timers have been processed, so just raise the flag here.
    //The actual update will take place after the processing loop.
    if (m_processing) {
        m_post_process_clock_update = true;
        return;
    }

    double old_ref_freq = m_ref_clk_freq;

    //The reference clock is the domain 0 after prescaling
    clock_domain_t& refdom = m_clock_domains.at(ReferenceDomain);
    clock_source_t& refsrc = m_clock_sources.at(refdom.src);
    m_ref_clk_freq = (refsrc.frequency * (double)refdom.ps_mul) / (double)refdom.ps_div;

    //If the reference frequency is changed
    if (m_ref_clk_freq != old_ref_freq) {
        //Update the reference point cycles/time
        m_ref_time += (m_cycle - m_ref_cycle) / old_ref_freq;
        m_ref_cycle = m_cycle;
    }

    //For each other clock domain, calculate the cycle factor which is Freq(ref domain) / Freq(domain i)
    //We keep a copy of the old cycle factor values
    std::unordered_map<sim_id_t, double> old_cycle_factors;
    for (auto& [dom_id, dom_cfg] : m_clock_domains) {
        auto& clksrc = m_clock_sources.at(dom_cfg.src);
        old_cycle_factors[dom_id] = dom_cfg.cycle_factor;
        if (clksrc.frequency && dom_cfg.ps_mul)
            dom_cfg.cycle_factor = (m_ref_clk_freq * (double)dom_cfg.ps_div) /
                                  (clksrc.frequency * (double)dom_cfg.ps_mul);
        else
            dom_cfg.cycle_factor = 0.0;
    }

    //Sanity check, we shouldn't allow the reference frequency to be zero anyway
    if (!m_ref_clk_freq) return;

    //Update all the cycle timers
    for (auto t : m_timer_slots) {

        if (t->m_state & STATE_SECS) {
            //The following calculations aim at preserving the timer duration in seconds
            //despite the change in reference frequency
            double old_rem_delay;
            if (t->m_state & STATE_PAUSED)
                old_rem_delay = t->m_when_d;
            else
                old_rem_delay = t->m_when_d - m_cycle;

            double new_rem_delay = old_rem_delay * old_ref_freq / m_ref_clk_freq;

            if (t->m_state & STATE_PAUSED)
                t->m_when_d = new_rem_delay;
            else
                t->m_when_d = m_cycle + new_rem_delay;

            t->m_when = std::ceil(t->m_when_d);
        }

        //The following is pointless if the timer uses the reference domain
        else if (t->m_clock_domain != ReferenceDomain) {

            double old_cycle_factor = old_cycle_factors.at(t->m_clock_domain);

            double rem_dom_cycles;
            if (t->m_state & STATE_NOCLOCK) {
                rem_dom_cycles = t->m_when_d;
            } else if (t->m_state & STATE_PAUSED) {
                rem_dom_cycles = t->m_when_d / old_cycle_factor;
            } else {
                rem_dom_cycles = (t->m_when_d - m_cycle) / old_cycle_factor;
            }

            clock_domain_t& clkdom = m_clock_domains.at(t->m_clock_domain);
            if (!clkdom.cycle_factor) {
                t->m_state |= STATE_NOCLOCK;
                t->m_when_d = rem_dom_cycles;
            } else {
                t->m_state &= ~STATE_NOCLOCK;
                double rem_ref_cycles = rem_dom_cycles * clkdom.cycle_factor;
                if (t->m_state & STATE_PAUSED) {
                    t->m_when_d = rem_ref_cycles;
                } else {
                    t->m_when_d = m_cycle + rem_ref_cycles;
                }
            }

            t->m_when = std::ceil(t->m_when_d);
        }
    }
}


void CycleManager::add_to_queue(CycleTimer& timer)
{
    //Add a timer slot to the queue
    //The timers are ordered in chronological order (in cycle count), the front being
    //the first timer to be called.
    //The inactive (i.e. paused) timers are placed after all the active timers.
    //Here, we use an insertion sort algorithm.
    for (auto it = m_timer_slots.begin(); it != m_timer_slots.end(); ++it) {
        if ((timer.m_when < (*it)->m_when) || ((*it)->m_state & STATE_HALTED)) {
            m_timer_slots.insert(it, &timer);
            return;
        }
    }
    m_timer_slots.push_back(&timer);
}


bool CycleManager::pop_from_queue(CycleTimer& timer)
{
    auto it = std::find(m_timer_slots.begin(), m_timer_slots.end(), &timer);
    if (it != m_timer_slots.end()) {
        m_timer_slots.erase(it);
        return true;
    } else {
        return false;
    }
}


void CycleManager::delay(CycleTimer& timer, cycle_count_t count)
{
    cycle_count_t ref_cycle = m_processing ? m_processed_when : m_cycle;

    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    timer.m_state &= ~STATE_PAUSED;

    if (timer.m_clock_domain == ReferenceDomain) {

        timer.m_when = ref_cycle + count;
        timer.m_when_d = timer.m_when;

    } else {

        auto it = m_clock_domains.find(timer.m_clock_domain);
        double domain_factor;
        if (it != m_clock_domains.end())
            domain_factor = it->second.cycle_factor;
        else
            domain_factor = 0.0;

        if (domain_factor) {
            timer.m_when_d = ref_cycle + count * domain_factor;
            timer.m_when = std::ceil(timer.m_when_d);
        } else {
            timer.m_state |= STATE_NOCLOCK;
            timer.m_when = count;
            timer.m_when_d = count;
        }

    }

    timer.m_state |= STATE_SCHEDULED;

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


void CycleManager::delay_s(CycleTimer& timer, double secs)
{
    cycle_count_t ref_cycle = m_processing ? m_processed_when : m_cycle;

    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    timer.m_state = (timer.m_state & ~STATE_PAUSED) | STATE_SCHEDULED | STATE_SECS;
    timer.m_when_d = ref_cycle + secs * m_ref_clk_freq;
    timer.m_when = std::ceil(timer.m_when_d);

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


void CycleManager::cancel(CycleTimer& timer)
{
    if (timer.m_state & STATE_PROCESSING) {
        timer.m_state &= STATE_PROCESSING; //all other flags are cleared
    } else {
        pop_from_queue(timer);
        timer.m_state = 0;
    }
}


/**
   Pause a timer.

   The timer stays in the queue but won't be called until it's resumed.
   The remaining delay until the timer 'when' is conserved during the pause.
   \sa resume
 */
void CycleManager::pause(CycleTimer& timer)
{
    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    if (!(timer.m_state & STATE_NOCLOCK)) {
        timer.m_when_d = (timer.m_when > m_cycle) ? (timer.m_when_d - m_cycle) : 0.0;
        timer.m_when = (timer.m_when > m_cycle) ? (timer.m_when - m_cycle) : 0;
    }

    timer.m_state |= STATE_PAUSED;

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


void CycleManager::resume(CycleTimer& timer)
{
    if (!(timer.m_state & STATE_PROCESSING))
        pop_from_queue(timer);

    if (!(timer.m_state & STATE_NOCLOCK)) {
        timer.m_when_d = timer.m_when_d + m_cycle;
        timer.m_when = timer.m_when + m_cycle;
    }

    timer.m_state &= ~STATE_PAUSED;

    if (!(timer.m_state & STATE_PROCESSING))
        add_to_queue(timer);
}


/**
   Process the timers for the current cycle.
 */
void CycleManager::process_timers()
{
    m_processing = true;

    //Loops until either the timer queue is empty or the front timer is paused or its 'when' is in the future
    while(!m_timer_slots.empty()) {
        CycleTimer* t = m_timer_slots.front();

        //Paused timers are last in the queue. It means we have no more active timer.
        if ((t->m_when > m_cycle) || (t->m_state & STATE_HALTED)) break;

        //Remove the timer from the front of the queue
        m_timer_slots.pop_front();
        t->m_state = STATE_PROCESSING;

        //Calling the timer's callback
        m_processed_when = t->m_when;
        t->next(m_processed_when);

        //return the timer to the queue if it had been rescheduled
        if (t->m_state & STATE_SCHEDULED)
            add_to_queue(*t);
        else
            t->m_state = 0;
    }

    m_processing = false;

    if (m_post_process_clock_update) {
        m_post_process_clock_update = false;
        update_clocks();
    }

}

/**
   \return the next cycle at which a timer is scheduled to be called,
   or INVALID_CYCLE if no timer is scheduled or all scheduled timers are
   paused.
 */
cycle_count_t CycleManager::next_when() const
{
    if (m_timer_slots.empty())
        return INVALID_CYCLE;

    CycleTimer* t = m_timer_slots.front();
    if (t->m_state & STATE_HALTED)
        return INVALID_CYCLE;
    else
        return t->m_when;
}


void CycleManager::copy_slot(const CycleTimer& src, CycleTimer& dst)
{
    dst.m_clock_domain = src.m_clock_domain;
    dst.m_state = src.m_state;
    dst.m_when_d = src.m_when_d;
    dst.m_when = src.m_when;

    if (dst.m_state)
        add_to_queue(dst);
}
