/*
 * cycle_timer.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

class CycleTimer {
%TypeHeaderCode
#include "core/sim_cycle_timer.h"
%End

public:

    CycleTimer();

    bool scheduled() const;
    bool paused() const;
    cycle_count_t remaining_delay() const;

    virtual cycle_count_t next(cycle_count_t when) = 0;

};


class CycleManager /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_cycle_timer.h"
%End

public:

    CycleManager();

    cycle_count_t cycle() const;
    void increment_cycle(cycle_count_t);

    void schedule(CycleTimer&, cycle_count_t);
    void delay(CycleTimer&, cycle_count_t);
    void cancel(CycleTimer&);
    void pause(CycleTimer&);
    void resume(CycleTimer&);
    void process_timers();
    cycle_count_t next_when() const;

};
