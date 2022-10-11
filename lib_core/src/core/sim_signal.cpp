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


//=======================================================================================

AVR_SignalHook::~AVR_SignalHook()
{
    //A temporary vector is required because m_signals is
    //modified by disconnect_hook()
    std::vector<AVR_Signal*> v = m_signals;
    for (AVR_Signal* signal : v)
        signal->disconnect_hook(this);
}

AVR_Signal::AVR_Signal()
:m_busy(false)
{}

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
    raise_u(sigid, 0, 0);
}

void AVR_Signal::raise(const signal_data_t& value)
{
    if (m_busy) return;
    m_busy = true;

    m_data = value;

    //Notify the registered callbacks
    for (auto slot : m_hooks)
        slot.hook->raised(value, slot.tag);

    m_busy = false;
}

void AVR_Signal::raise_u(uint16_t sigid, uint32_t index, uint32_t u)
{
    signal_data_t sigdata = { .sigid = sigid, .index = index, .data = u };
    raise(sigdata);
}

void AVR_Signal::raise_d(uint16_t sigid, uint32_t index, double d)
{
    signal_data_t sigdata = { .sigid = sigid, .index = index, .data = d };
    raise(sigdata);
}

void AVR_Signal::raise(uint16_t sigid, uint32_t index, void* p)
{
    signal_data_t sigdata = { .sigid = sigid, .index = index, .data = p };
    raise(sigdata);
}

void AVR_Signal::raise(uint16_t sigid, uint32_t index, const char* s)
{
    signal_data_t sigdata = { .sigid = sigid, .index = index, .data = s };
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
