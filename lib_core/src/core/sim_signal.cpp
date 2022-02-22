/*
 * sim_signal.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

AVR_Signal::AVR_Signal()
:m_busy(false)
{}

void AVR_Signal::connect_hook(AVR_SignalHook* hook, uint16_t hooktag)
{
    if (hook_index(hook) == -1) {
    	HookSlot_t slot = { hook, hooktag };
        m_hooks.push_back(slot);
    }
}

void AVR_Signal::disconnect_hook(AVR_SignalHook* hook)
{
    int index = hook_index(hook);
    if (index > -1)
    	m_hooks.erase(m_hooks.begin() + index);
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
	signal_data_t sigdata = { .sigid = sigid, .index = index, .u = u };
	raise(sigdata);
}

void AVR_Signal::raise_d(uint16_t sigid, uint32_t index, double d)
{
	signal_data_t sigdata = { .sigid = sigid, .index = index, .d = d };
	raise(sigdata);
}

void AVR_Signal::raise(uint16_t sigid, uint32_t index, void* p)
{
	signal_data_t sigdata = { .sigid = sigid, .index = index, .p = p };
	raise(sigdata);
}

void AVR_Signal::raise(uint16_t sigid, uint32_t index, const char* s)
{
	signal_data_t sigdata = { .sigid = sigid, .index = index, .s = s };
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
