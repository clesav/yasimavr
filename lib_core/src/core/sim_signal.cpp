/*
 * sim_signal.cpp
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

#include "sim_signal.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

/**
   Copy construction ensuring the connection with signals is consistent.
 */
SignalHook::SignalHook(const SignalHook& other)
{
    for (auto [tag, signal] : other.m_signals)
        signal->connect(*this, tag);
}


SignalHook::~SignalHook()
{
    //A temporary vector copy is required because m_signals is
    //modified by disconnect()
    std::unordered_map<int, Signal*> v = m_signals;
    for (auto [_, signal] : v)
        signal->disconnect(*this);
}


vardata_t SignalHook::data(int hooktag, int sigid, long long index) const
{
    auto it = m_signals.find(hooktag);
    if (it == m_signals.end())
        return vardata_t();

    if (!it->second->is_data_signal())
        return vardata_t();

    const DataSignal* s = dynamic_cast<const DataSignal*>(it->second);
    return s->data(sigid, index);
}


bool SignalHook::has_data(int hooktag, int sigid, long long index) const
{
    auto it = m_signals.find(hooktag);
    if (it == m_signals.end())
        return false;

    if (!it->second->is_data_signal())
        return false;

    const DataSignal* s = dynamic_cast<const DataSignal*>(it->second);
    return s->has_data(sigid, index);
}


void SignalHook::add_filter(int hooktag, int sigid)
{
    auto it = m_filters.find(hooktag);
    if (it == m_filters.end())
        m_filters[hooktag] = { { sigid } };
    else
        it->second.push_back({ sigid });
}


void SignalHook::add_filter(int hooktag, int sigid, long long index)
{
    auto it = m_filters.find(hooktag);
    if (it == m_filters.end())
        m_filters[hooktag] = { { sigid, index } };
    else
        it->second.push_back({ sigid, index });
}


bool SignalHook::filter(int hooktag, int sigid, long long index) const
{
    auto it = m_filters.find(hooktag);
    if (it == m_filters.end())
        return true;

    auto& fv = it->second;
    for (auto& f : fv) {
        if (sigid == f.sigid && (!f.index.has_value() || index == f.index.value()))
           return true;
    }
    return false;
}


//=======================================================================================

Signal::Signal()
:m_busy(false)
{}


/**
   Copy construction ensuring the connection with hooks is consistent.
 */
Signal::Signal(const Signal& other)
:m_busy(false)
{
    for (auto& slot : other.m_hooks)
        connect(*slot.hook, slot.tag);
}


Signal::~Signal()
{
    for (auto& slot : m_hooks)
        slot.hook->m_signals.erase(slot.tag);
}

/**
   Connect a hook to this signal.
   \param hook hook to be connected. If the hook is already connected with the same tag, the call
   has no effect.
   \param hooktag identifier given to the hook when calling it. It has only a meaning
   for the hook and is passed though by the signal when called.
   \note The hooktag can be useful when a single hook connects to several signals,
   in order to differentiate which one the raise comes from.
   \sa SignalHook::raised()
 */
void Signal::connect(SignalHook& hook, int hooktag)
{
    for (auto& s : m_hooks) {
        if (s.hook == &hook && s.tag == hooktag) return;
    }

    m_hooks.push_back({ &hook, hooktag });
    hook.m_signals[hooktag] = this;
}


/**
   Disconnect a hook to this signal.
   \param hook hook to be disconnected.
 */
void Signal::disconnect(SignalHook& hook)
{
    //Remove 'this' from the hook's signal map
    std::erase_if(hook.m_signals, [&](const auto& p) -> bool { return p.second == this; });
    //Remove the hook from the hook map
    std::erase_if(m_hooks, [&](const hook_slot_t& slot) -> bool { return slot.hook == &hook; });
}


/**
   Raise the signal with the given data
   \param sigdata
 */
void Signal::raise(const signal_data_t& sigdata)
{
    if (m_busy) return;
    m_busy = true;

    //Notify the registered callbacks
    for (auto& slot : m_hooks) {
        if (slot.hook->filter(slot.tag, sigdata.sigid, sigdata.index))
            slot.hook->raised(sigdata, slot.tag);
    }

    m_busy = false;
}


/**
   Raise the signal with the given data.
   \param data
 */
void Signal::raise(int sigid, const vardata_t& v, long long ix)
{
    signal_data_t sigdata = { sigid, ix, v };
    raise(sigdata);
}


bool Signal::is_data_signal() const
{
    return false;
}


//=======================================================================================

/**
   Returns the data stored by the data signal for the given SIGID and index
   If no data is stored, an invalid vardata_t is returned.
 */
vardata_t DataSignal::data(int sigid, long long index) const
{
    key_t k = { sigid, index };
    auto it = m_data.find(k);
    if (it == m_data.end())
        return vardata_t();
    else
        return it->second;
}


/**
   Returns whether the signals has data given SIGID and index.
 */
bool DataSignal::has_data(int sigid, long long index) const
{
    key_t k = { sigid, index };
    return m_data.find(k) != m_data.end();
}


/**
   Deletes all data stored by the signal.
 */
void DataSignal::clear()
{
    m_data.clear();
}


void DataSignal::raise(const signal_data_t& sigdata)
{
    key_t k = { sigdata.sigid, sigdata.index };
    m_data[k] = sigdata.data;
    Signal::raise(sigdata);
}


bool DataSignal::key_t::operator==(const key_t& other) const
{
    return sigid == other.sigid && index == other.index;
}


size_t DataSignal::keyhash_t::operator()(const key_t& k) const
{
    return ((long long)k.sigid) ^ k.index;
}


bool DataSignal::is_data_signal() const
{
    return true;
}
