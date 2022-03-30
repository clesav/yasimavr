/*
 * sim_signal.h
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

#ifndef __YASIMAVR_SIGNAL_H__
#define __YASIMAVR_SIGNAL_H__

#include "sim_types.h"
#include <stdint.h>
#include <vector>


//=======================================================================================
/*
 * Generic structure for the data passed on when raising a signal
 */
struct signal_data_t {
	
	uint16_t sigid;
	uint32_t index;
	vardata_t data;

};


//=======================================================================================
/*
 * Generic hook class to connect to a signal in order to receive notifications
 * To be used, reimplement raised() to use the data from the signal
 */
class AVR_Signal;

class AVR_SignalHook {

public:

	virtual ~AVR_SignalHook();

	virtual void raised(const signal_data_t& data, uint16_t hooktag) = 0;

private:

	friend class AVR_Signal;

	std::vector<AVR_Signal*> m_signals;

};


//=======================================================================================
/*
 * AVR_Signal is a means for point-to-point communication and notification
 * across the simulator.
 * It is similar in use to simavr's IRQs
*/
class DLL_EXPORT AVR_Signal {

public:

	AVR_Signal();
	~AVR_Signal();

	////The hooktag is an arbitrary value that only has a meaning
	//for the hook and is passed though by the signal when calling
	//the hook's "raised()". It can be useful when a single hook
	//connects to several signals, in order to differentiate which
	//one the raise comes from.
    void connect_hook(AVR_SignalHook* hook, uint16_t hooktag = 0);
    void disconnect_hook(AVR_SignalHook* hook);

	//Returns the data raised last
    const signal_data_t& data() const;

	//Raise a signal with default data
	void raise();
    void raise(const signal_data_t& value);
	//Various override for simplicity
    void raise(uint16_t sigid);
	void raise(uint16_t sigid, uint32_t index, void* p);
	void raise(uint16_t sigid, uint32_t index, const char* s);
	////The different names are necessary to remove ambiguity at compilation
    void raise_u(uint16_t sigid, uint32_t index, uint32_t u);
    void raise_d(uint16_t sigid, uint32_t index, double d);

private:

	//Flag used to avoid nested raises
    bool m_busy;
	//Stores a copy of the last data used to raise the signal
    signal_data_t m_data;

    struct hook_slot_t {
    	AVR_SignalHook* hook;
    	uint16_t tag;
    };

    std::vector<hook_slot_t> m_hooks;

    int hook_index(const AVR_SignalHook* hook) const;
    int signal_index(const AVR_SignalHook* hook) const;

};

inline const signal_data_t& AVR_Signal::data() const
{
	return m_data;
}

#endif //__YASIMAVR_SIGNAL_H__
