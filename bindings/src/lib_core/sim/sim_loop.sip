/*
 * sim_loop.sip
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

class AbstractSimLoop /NoDefaultCtors, Abstract/ {
%TypeHeaderCode
#include "sim/sim_loop.h"
%End

public:

    enum State {
        State_Running       /PyName=Running/,
        State_Step          /PyName=Step/,
        State_Standby       /PyName=Standby/,
        State_Stopped       /PyName=Stopped/,
        State_Done          /PyName=Done/,
    };

    AbstractSimLoop(Device&);

    AbstractSimLoop::State state() const;
    cycle_count_t cycle() const;
    CycleManager& cycle_manager();
    const Device& device() const /NoCopy/;

};

class SimLoop : public AbstractSimLoop /NoDefaultCtors/ {
%TypeHeaderCode
#include "sim/sim_loop.h"
%End

public:

    SimLoop(Device&);

    void set_fast_mode(bool);
    void run(cycle_count_t = 0) /ReleaseGIL/;

};


class AsyncSimLoop : public AbstractSimLoop /NoDefaultCtors/ {
%TypeHeaderCode
#include "sim/sim_loop.h"
%End

public:

    AsyncSimLoop(Device&);

    void set_fast_mode(bool);

    void run() /ReleaseGIL/;

    bool start_transaction() /ReleaseGIL/;
    void end_transaction();

    void loop_continue();
    void loop_step();
    void loop_pause();
    void loop_kill();

    SIP_PYOBJECT __enter__();
    %MethodCode
        Py_BEGIN_ALLOW_THREADS
        sipCpp->start_transaction();
        Py_END_ALLOW_THREADS

        sipRes = sipSelf;
        Py_INCREF(sipRes);
    %End

    void __exit__(SIP_PYOBJECT, SIP_PYOBJECT, SIP_PYOBJECT);
    %MethodCode
        sipCpp->end_transaction();
    %End

};
