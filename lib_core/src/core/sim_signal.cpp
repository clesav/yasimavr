/*
 * sim_signal.cpp
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

#include "sim_signal.h"
#include <assert.h>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

SignalHook::SignalHook(const SignalHook& other)
{
    *this = other;
}


SignalHook::~SignalHook()
{
    //A temporary vector is required because m_signals is
    //modified by disconnect()
    std::vector<Signal*> v = m_signals;
    for (Signal* signal : v)
        signal->disconnect(*this);
}


SignalHook& SignalHook::operator=(const SignalHook& other)
{
    for (Signal* signal : other.m_signals) {
        std::vector<Signal::hook_slot_t> hook_slots = signal->m_hooks;
        for (auto slot : hook_slots) {
            if (slot.hook == &other)
                signal->connect(*this, slot.tag);
        }
    }
    return *this;
}


Signal::Signal()
:m_busy(false)
{}


Signal::Signal(const Signal& other)
:m_busy(false)
{
    for (auto& slot : other.m_hooks)
        connect(*slot.hook, slot.tag);
}


Signal::~Signal()
{
    std::vector<hook_slot_t> hook_slots = m_hooks;
    for (auto& slot : hook_slots) {
        int i = signal_index(*slot.hook);
        slot.hook->m_signals.erase(slot.hook->m_signals.begin() + i);
    }
}


void Signal::connect(SignalHook& hook, int hooktag)
{
    if (hook_index(hook) == -1) {
        hook_slot_t slot = { &hook, hooktag };
        m_hooks.push_back(slot);
        hook.m_signals.push_back(this);
    }
}


void Signal::disconnect(SignalHook& hook)
{
    int h_index = hook_index(hook);
    if (h_index > -1) {
        m_hooks.erase(m_hooks.begin() + h_index);
        int sig_index = signal_index(hook);
        hook.m_signals.erase(hook.m_signals.begin() + sig_index);
    }
}


void Signal::raise(const signal_data_t& sigdata)
{
    if (m_busy) return;
    m_busy = true;

    //Notify the registered callbacks
    for (auto& slot : m_hooks)
        slot.hook->raised(sigdata, slot.tag);

    m_busy = false;
}


void Signal::raise(int sigid, const vardata_t& v, long long ix)
{
    signal_data_t sigdata = { sigid, ix, v };
    raise(sigdata);
}


int Signal::hook_index(const SignalHook& hook) const
{
    int index = 0;
    for (auto& slot : m_hooks) {
        if (slot.hook == &hook)
            return index;
        index++;
    }
    return -1;
}


int Signal::signal_index(const SignalHook& hook) const
{
    int index = 0;
    for (auto s : hook.m_signals) {
        if (s == this)
            return index;
        index++;
    }
    return -1;
}


Signal& Signal::operator=(const Signal& other)
{
    assert(!m_busy);

    //Disconnect from all current hooks
    std::vector<hook_slot_t> hook_slots = m_hooks;
    for (auto& slot : hook_slots) {
        int i = signal_index(*slot.hook);
        slot.hook->m_signals.erase(slot.hook->m_signals.begin() + i);
    }

    //Copy all connections
    for (auto& slot : other.m_hooks)
        connect(*slot.hook, slot.tag);

    return *this;
}


//=======================================================================================

vardata_t DataSignal::data(int sigid, long long index) const
{
    key_t k = { sigid, index };
    auto it = m_data.find(k);
    if (it == m_data.end())
        return vardata_t();
    else
        return it->second;
}


bool DataSignal::has_data(int sigid, long long index) const
{
    key_t k = { sigid, index };
    return m_data.find(k) != m_data.end();
}


void DataSignal::set_data(int sigid, const vardata_t& v, long long index)
{
    key_t k = { sigid, index };
    m_data[k] = v;
}


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


//=======================================================================================

#define FILT_SIGID          1
#define FILT_INDEX          2


DataSignalMux::DataSignalMux()
:m_sel_index(0)
{}

void DataSignalMux::raised(const signal_data_t& sigdata, int hooktag)
{
    if (hooktag == m_sel_index && m_sel_index < m_items.size() && m_items[m_sel_index].match(sigdata)) {
        m_items[m_sel_index].data = sigdata.data;
        m_signal.raise({ 0, 0, sigdata.data });
    }
}


size_t DataSignalMux::add_mux()
{
    mux_item_t item = { nullptr, 0, 0, 0 };
    return add_mux(item);
}


size_t DataSignalMux::add_mux(DataSignal& signal)
{
    mux_item_t item = { &signal,  0, 0, 0 };
    return add_mux(item);
}


size_t DataSignalMux::add_mux(DataSignal& signal, int sigid_filt)
{
    mux_item_t item = { &signal, sigid_filt, 0, FILT_SIGID };
    return add_mux(item);
}


size_t DataSignalMux::add_mux(DataSignal& signal, int sigid_filt, long long ix_filt)
{
    mux_item_t item = { &signal, sigid_filt, ix_filt, FILT_SIGID | FILT_INDEX };
    return add_mux(item);
}


size_t DataSignalMux::add_mux(mux_item_t& item)
{
    size_t index = m_items.size();
    if (item.signal) {
        item.signal->connect(*this, index);
        if (!index) {
            item.data = item.signal->data(item.sigid_filt, item.index_filt);
            m_signal.set_data(0, item.data, 0);
        }
    }
    m_items.push_back(item);
    return index;
}


void DataSignalMux::set_selection(size_t index)
{
    if (index < m_items.size() && index != m_sel_index) {
        m_sel_index = index;
        m_signal.raise({ 0, 0, m_items[index].data });
    }
}


bool DataSignalMux::mux_item_t::match(const signal_data_t& sigdata) const
{
    return (sigdata.sigid == sigid_filt || !(filt_mask & FILT_SIGID)) &&
           (sigdata.index == index_filt || !(filt_mask & FILT_INDEX));
}
