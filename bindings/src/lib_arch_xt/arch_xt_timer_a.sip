/*
 * arch_xt_timer_a.sip
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

%ModuleCode

#include "arch_xt_timer_a.h"
const ctlreq_id_t CTLREQ_TCA_REGISTER_TCB = AVR_CTLREQ_TCA_REGISTER_TCB;
const ctlreq_id_t CTLREQ_TCA_GET_EVENT_HOOK = AVR_CTLREQ_TCA_GET_EVENT_HOOK;

%End


//=======================================================================================

const ctlreq_id_t CTLREQ_TCA_REGISTER_TCB;
const ctlreq_id_t CTLREQ_TCA_GET_EVENT_HOOK;


struct ArchXT_TimerAConfig {
%TypeHeaderCode
#include "arch_xt_timer_a.h"
%End
%TypeCode
#include "utils/buffer_utils.h"
%End

    static const int CompareChannelCount;

    enum Version {
        V1,
        V2
    };

    reg_addr_t reg_base;

    int_vect_t iv_ovf;
    int_vect_t iv_hunf;

    //int_vect_t vect_cmp[AVR_TCA_CMP_CHANNEL_COUNT];
    SIP_PYBUFFER ivs_cmp {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->ivs_cmp, "h", sipCpp->CompareChannelCount, 0);
        %End
        %SetCode
            sipErr = import_from_fixedlen_sequence(sipAPI_arch_xt, sipCpp->ivs_cmp, "h", sipCpp->CompareChannelCount, sipPy);
        %End
    };

    Version version;

};


class ArchXT_TimerA : public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_timer_a.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_CompareOutput         /PyName=CompareOutput/
    };

    enum EventHookTag /BaseType=IntEnum/ {
        Hook_EventA             /PyName=EventA/,
        Hook_EventB             /PyName=EventB/
    };

    ArchXT_TimerA(const ArchXT_TimerAConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, SleepMode);
    virtual void raised(const signal_data_t&, int);

};
