/*
 * cycle_timer.sip
 *
 *	Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

class AVR_CycleTimer {
%TypeHeaderCode
#include "core/sim_cycle_timer.h"
%End

public:

    AVR_CycleTimer();
    virtual ~AVR_CycleTimer();
    
    bool scheduled() const;
    
    virtual cycle_count_t next(cycle_count_t when) = 0;

};


class AVR_CycleManager {
%TypeHeaderCode
#include "core/sim_cycle_timer.h"
%End

public:

    AVR_CycleManager();
	~AVR_CycleManager();
    
	cycle_count_t cycle() const;
	void increment_cycle(cycle_count_t);
	
    void add_cycle_timer(AVR_CycleTimer*, cycle_count_t);
    void remove_cycle_timer(AVR_CycleTimer*);
    void reschedule_cycle_timer(AVR_CycleTimer*, cycle_count_t);
    void pause_cycle_timer(AVR_CycleTimer*);
	void resume_cycle_timer(AVR_CycleTimer*);
	void process_cycle_timers();
	cycle_count_t next_when() const;

};
