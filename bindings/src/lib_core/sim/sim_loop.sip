/*
 * simulation_loop.sip
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


class AVR_AbstractSimLoop /NoDefaultCtors, Abstract/ {
%TypeHeaderCode
#include "sim/sim_loop.h"
%End

public:

    enum State {
		State_Running,
		State_Step,
		State_Stopped,
		State_Done
    };

    class Hook : public AVR_CycleTimer /Abstract/ {

    public:

    	Hook(AVR_AbstractSimLoop&);
    	Hook(AVR_AbstractSimLoop&, cycle_count_t);

    	cycle_count_t next(cycle_count_t when, bool* /Out/) = 0;

    };

    AVR_AbstractSimLoop(AVR_Device&);
    ~AVR_AbstractSimLoop();

    AVR_AbstractSimLoop::State state() const;
    cycle_count_t cycle() const;
    AVR_CycleManager& cycle_manager();
	const AVR_Device& device() const /NoCopy/;

};

class AVR_SimLoop : public AVR_AbstractSimLoop /NoDefaultCtors/ {
%TypeHeaderCode
#include "sim/sim_loop.h"
%End

public:

	AVR_SimLoop(AVR_Device&);

	void set_fast_mode(bool);
	void run(cycle_count_t = 0) /ReleaseGIL/;

};


class AVR_AsyncSimLoop : public AVR_AbstractSimLoop /NoDefaultCtors/ {
%TypeHeaderCode
#include "sim/sim_loop.h"
%End

public:

	AVR_AsyncSimLoop(AVR_Device&);

	void run() /ReleaseGIL/;

	bool start_transaction() /ReleaseGIL/;
	void end_transaction();
	
	void loop_continue();
	void loop_step();
	void loop_pause();
	void loop_kill();

};