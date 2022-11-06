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


//=======================================================================================

AVR_SignalHook::AVR_SignalHook(const AVR_SignalHook& other)
{
    *this = other;
}


AVR_SignalHook::~AVR_SignalHook()
{
    //A temporary vector is required because m_signals is
    //modified by disconnect_hook()
    std::vector<AVR_Signal*> v = m_signals;
    for (AVR_Signal* signal : v)
        signal->disconnect_hook(this);
}


AVR_SignalHook& AVR_SignalHook::operator=(const AVR_SignalHook& other)
{
    for (AVR_Signal* signal : other.m_signals) {
        std::vector<AVR_Signal::hook_slot_t> hook_slots = signal->m_hooks;
        for (auto slot : hook_slots) {
            if (slot.hook == &other)
                signal->connect_hook(this, slot.tag);
        }
    }
    return *this;
}


AVR_Signal::AVR_Signal()
:m_busy(false)
{}


AVR_Signal::AVR_Signal(const AVR_Signal& other)
:m_busy(false)
{
    for (auto slot : other.m_hooks)
        connect_hook(slot.hook, slot.tag);
}


AVR_Signal::~AVR_Signal()
{
    std::vector<hook_slot_t> hook_slots = m_hooks;
    for (auto slot : hook_slots) {
        int i = signal_index(slot.hook);
        slot.hook->m_signals.erase(slot.hook->m_signals.begin() + i);
    }
}


void AVR_Signal::connect_hook(AVR_SignalHook* hook, uint16_t hooktag)
{
    if (hook_index(hook) == -1) {
        hook_slot_t slot = { hook, hooktag };
        m_hooks.push_back(slot);
        hook->m_signals.push_back(this);
    }
}


void AVR_Signal::disconnect_hook(AVR_SignalHook* hook)
{
    int h_index = hook_index(hook);
    if (h_index > -1) {
        m_hooks.erase(m_hooks.begin() + h_index);
        int sig_index = signal_index(hook);
        hook->m_signals.erase(hook->m_signals.begin() + sig_index);
    }
}


void AVR_Signal::raise()
{
    signal_data_t __unused__ = signal_data_t();
    raise(__unused__);
}


void AVR_Signal::raise(uint16_t sigid)
{
    raise_u(sigid, 0);
}


void AVR_Signal::raise(const signal_data_t& sigdata)
{
    if (m_busy) return;
    m_busy = true;

    //Notify the registered callbacks
    for (auto slot : m_hooks)
        slot.hook->raised(sigdata, slot.tag);

    m_busy = false;
}


void AVR_Signal::raise_u(uint16_t sigid, uint32_t u, uint32_t index)
{
    signal_data_t sigdata = { sigid, index, u };
    raise(sigdata);
}


void AVR_Signal::raise_d(uint16_t sigid, double d, uint32_t index)
{
    signal_data_t sigdata = { sigid, index, d };
    raise(sigdata);
}


void AVR_Signal::raise(uint16_t sigid, void* p)
{
    signal_data_t sigdata = { sigid, 0, p };
    raise(sigdata);
}


void AVR_Signal::raise(uint16_t sigid, const char* s)
{
    signal_data_t sigdata = { sigid, 0, s };
    raise(sigdata);
}


void AVR_Signal::raise(uint16_t sigid, vardata_t v)
{
    signal_data_t sigdata = { sigid, 0, v };
    raise(sigdata);
}


int AVR_Signal::hook_index(const AVR_SignalHook* hook) const
{
    int index = 0;
    for (auto slot : m_hooks) {
        if (slot.hook == hook)
            return index;
        index++;
    }
    return -1;
}


int AVR_Signal::signal_index(const AVR_SignalHook* hook) const
{
    int index = 0;
    for (auto s : hook->m_signals) {
        if (s == this)
            return index;
        index++;
    }
    return -1;
}


AVR_Signal& AVR_Signal::operator=(const AVR_Signal& other)
{
    assert(!m_busy);

    //Disconnect from all current hooks
    std::vector<hook_slot_t> hook_slots = m_hooks;
    for (auto slot : hook_slots) {
        int i = signal_index(slot.hook);
        slot.hook->m_signals.erase(slot.hook->m_signals.begin() + i);
    }

    //Copy all connections
    for (auto slot : other.m_hooks)
        connect_hook(slot.hook, slot.tag);

    return *this;
}


//=======================================================================================

vardata_t AVR_DataSignal::data(uint16_t sigid, uint32_t index) const
{
    key_t k = { sigid, index };
    auto it = m_data.find(k);
    if (it == m_data.end())
        return vardata_t();
    else
        return it->second;
}


bool AVR_DataSignal::has_data(uint16_t sigid, uint32_t index) const
{
    key_t k = { sigid, index };
    return m_data.find(k) != m_data.end();
}


void AVR_DataSignal::set_data(uint16_t sigid, vardata_t v, uint32_t index)
{
    key_t k = { sigid, index };
    m_data[k] = v;
}


void AVR_DataSignal::clear()
{
    m_data.clear();
}


void AVR_DataSignal::raise(const signal_data_t& sigdata)
{
    key_t k = { sigdata.sigid, sigdata.index };
    m_data[k] = sigdata.data;
    AVR_Signal::raise(sigdata);
}


bool AVR_DataSignal::key_t::operator==(const key_t& other) const
{
    return sigid == other.sigid && index == other.index;
}


size_t AVR_DataSignal::keyhash_t::operator()(const key_t& k) const
{
    return ((uint32_t)k.sigid) ^ k.index;
}
