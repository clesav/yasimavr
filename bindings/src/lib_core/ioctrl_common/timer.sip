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

class AVR_PrescaledTimer : public AVR_CycleTimer /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_timer.h"
%End

public:

    AVR_PrescaledTimer();
    virtual ~AVR_PrescaledTimer();

    void init(AVR_CycleManager&, AVR_Logger&);
    void reset();
    void set_prescaler(uint32_t, uint32_t);
    uint32_t prescaler_factor() const;
    void set_paused(bool);
    void set_timer_delay(uint32_t);
    uint32_t timer_delay() const;
    void update(cycle_count_t = -1);

    virtual cycle_count_t next(cycle_count_t);

    AVR_Signal& signal();

    void register_chained_timer(AVR_PrescaledTimer&);
    void unregister_chained_timer(AVR_PrescaledTimer&);

    static int ticks_to_event(int, int, int);

};


//=======================================================================================

class AVR_TimerCounter /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_timer.h"
%End

public:

    enum ClockSource {
        Clock_Stopped       /PyName=Stopped/,
        Clock_Internal      /PyName=Internal/,
        Clock_External      /PyName=External/,
    };

    enum EventType /BaseType=IntFlag/ {
        Event_Max           /PyName=Max/,
        Event_Top           /PyName=Top/,
        Event_Bottom        /PyName=Bottom/,
        Event_OC            /PyName=OC/,
    };

    enum SignalId  /BaseType=IntEnum/ {
        Signal_Event        /PyName=Event/,
        Signal_CompMatch    /PyName=CompMatch/,
    };

    AVR_TimerCounter(AVR_PrescaledTimer&, long, uint32_t);

    long wrap() const;

    void reset();
    void reschedule();

    void set_clock_source(ClockSource);

    void set_top(long);
    long top() const;

    void set_double_slope(bool);
    bool double_slope() const;

    void set_value(long);
    long value() const;

    void set_OC_value(uint32_t, long);
    long OC_value(uint32_t) const;

    bool countdown() const;

    AVR_Signal& signal();
    AVR_SignalHook& ext_clock_hook();

    void set_logger(AVR_Logger* logger);

};
