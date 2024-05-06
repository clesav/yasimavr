/*
 * interrupt.sip
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

#include "core/sim_interrupt.h"
const ctlreq_id_t CTLREQ_INTR_REGISTER = AVR_CTLREQ_INTR_REGISTER;
const ctlreq_id_t CTLREQ_INTR_RAISE    = AVR_CTLREQ_INTR_RAISE;
const int_vect_t INTERRUPT_NONE        = AVR_INTERRUPT_NONE;

%End


//=======================================================================================

const ctlreq_id_t CTLREQ_INTR_REGISTER;
const ctlreq_id_t CTLREQ_INTR_RAISE;
const int_vect_t INTERRUPT_NONE;


class InterruptController : public Peripheral /Abstract/ {
%TypeHeaderCode
#include "core/sim_interrupt.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_StateChange           /PyName=StateChange/,
    };

    enum State /BaseType=IntEnum/ {
        State_Raised                /PyName=Raised/,
        State_Cancelled             /PyName=Cancelled/,
        State_Acknowledged          /PyName=Acknowledged/,
        State_Returned              /PyName=Returned/,
        State_RaisedFromSleep       /PyName=RaisedFromSleep/,
        State_Reset                 /PyName=Reset/
    };

    struct IRQ_t {
        int_vect_t vector;
        flash_addr_t address;
        bool nmi;
    };

    static const IRQ_t NO_INTERRUPT;

    InterruptController(unsigned int);

    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual void sleep(bool, SleepMode);

};


class InterruptHandler /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_interrupt.h"
%End

public:

    InterruptHandler();

    void raise_interrupt(int_vect_t) const;
    void cancel_interrupt(int_vect_t) const;
    virtual void interrupt_ack_handler(int_vect_t);

};


class InterruptFlag : public InterruptHandler /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_interrupt.h"
%End

public:

    InterruptFlag(bool = false);

    bool init(Device&, const regbit_t&, const regbit_t&, int_vect_t);
    int update_from_ioreg();
    void set_clear_on_ack(bool);
    bool set_flag(uint8_t = 0xFF);
    bool clear_flag(uint8_t = 0xFF);
    bool raised() const;
    virtual void interrupt_ack_handler(int_vect_t);

};
