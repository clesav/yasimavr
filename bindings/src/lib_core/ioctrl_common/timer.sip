/*
 * timer.sip
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

class PrescaledTimer : public CycleTimer /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_timer.h"
%End

public:

    PrescaledTimer();
    virtual ~PrescaledTimer();

    void init(CycleManager&, Logger&);
    void reset();
    void set_prescaler(unsigned long, unsigned long);
    unsigned long prescaler_factor() const;
    void set_paused(bool);
    void set_timer_delay(cycle_count_t);
    cycle_count_t timer_delay() const;
    void update();

    virtual cycle_count_t next(cycle_count_t);

    Signal& signal();

    void register_chained_timer(PrescaledTimer&);
    void unregister_chained_timer(PrescaledTimer&);

    static cycle_count_t ticks_to_event(cycle_count_t, cycle_count_t, cycle_count_t);

};


//=======================================================================================

class TimerCounter /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_timer.h"
%End

public:

    enum TickSource {
        Tick_Stopped       /PyName=Stopped/,
        Tick_Timer         /PyName=Timer/,
        Tick_External      /PyName=External/,
    };

    enum SlopeMode {
        Slope_Up           /PyName=Up/,
        Slope_Down         /PyName=Down/,
        Slope_Double       /PyName=Double/,
    };

    enum EventType /BaseType=IntFlag/ {
        Event_Max           /PyName=Max/,
        Event_Top           /PyName=Top/,
        Event_Bottom        /PyName=Bottom/,
        Event_Compare       /PyName=Compare/,
    };

    enum SignalId  /BaseType=IntEnum/ {
        Signal_Event        /PyName=Event/,
        Signal_CompMatch    /PyName=CompMatch/,
    };

    TimerCounter(long, size_t);

    void init(CycleManager&, Logger&);

    void reset();
    void reschedule();
    void update();

    long wrap() const;

    void set_tick_source(TickSource);
    TickSource tick_source() const;

    void tick();

    void set_top(long);
    long top() const;

    void set_slope_mode(SlopeMode);
    SlopeMode slope_mode() const;

    void set_counter(long);
    long counter() const;

    void set_comp_value(size_t, long);
    long comp_value(size_t) const;

    void set_comp_enabled(size_t, bool);
    bool comp_enabled(size_t) const;

    bool countdown() const;
    void set_countdown(bool);

    Signal& signal();
    SignalHook& ext_tick_hook();

    PrescaledTimer& prescaler();

};
