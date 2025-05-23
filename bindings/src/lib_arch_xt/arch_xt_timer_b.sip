/*
 * arch_xt_timer_b.sip
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

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

%ModuleCode

#include "arch_xt_timer_b.h"
const ctlreq_id_t CTLREQ_TCB_GET_EVENT_HOOK = AVR_CTLREQ_TCB_GET_EVENT_HOOK;

%End

const ctlreq_id_t CTLREQ_TCB_GET_EVENT_HOOK;


//=======================================================================================

struct ArchXT_TimerBConfig {
%TypeHeaderCode
#include "arch_xt_timer_b.h"
%End

    enum Options /BaseType=IntFlag/ {
        EventCount,
        OverflowFlag,
    };

    reg_addr_t reg_base;
    int_vect_t iv_capt;
    int options;

};


class ArchXT_TimerB : public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_timer_b.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_Capture   /PyName=Capture/,
        Signal_Output    /PyName=Output/,
    };

    enum CaptureHookTag /BaseType=IntEnum/ {
        Tag_Event        /PyName=Event/,
        Tag_Count        /PyName=Count/,
    };

    ArchXT_TimerB(int, const ArchXT_TimerBConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual uint8_t ioreg_peek_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, SleepMode);
    virtual void raised(const signal_data_t&, int);

};
