/*
 * arch_avr_extint.sip
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

struct ArchAVR_ExtIntConfig {
%TypeHeaderCode
#include "arch_avr_extint.h"
#include "utils/buffer_utils.h"
%End

    //pin_id_t extint_pins[EXTINT_PIN_COUNT];
    SIP_PYBUFFER extint_pins {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->extint_pins, "I", EXTINT_PIN_COUNT, 0);
        %End
        %SetCode
            sipErr = import_from_fixedlen_sequence(sipAPI_arch_avr, sipCpp->extint_pins, "I", EXTINT_PIN_COUNT, sipPy);
        %End
    };

    //pin_id_t pcint_pins[PCINT_PIN_COUNT];
    SIP_PYBUFFER pcint_pins {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->pcint_pins, "I", PCINT_PIN_COUNT, 0);
        %End
        %SetCode
            sipErr = import_from_fixedlen_sequence(sipAPI_arch_avr, sipCpp->pcint_pins, "I", PCINT_PIN_COUNT, sipPy);
        %End
    };

    regbit_t rb_extint_ctrl;
    regbit_t rb_extint_mask;
    regbit_t rb_extint_flag;
    regbit_t rb_pcint_ctrl;
    regbit_t rb_pcint_flag;

    //reg_addr_t reg_pcint_mask[PCINT_BANK_COUNT];
    SIP_PYBUFFER reg_pcint_mask {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->reg_pcint_mask, "H", PCINT_BANK_COUNT, 0);
        %End
        %SetCode
            sipErr = import_from_fixedlen_sequence(sipAPI_arch_avr, sipCpp->reg_pcint_mask, "H", PCINT_BANK_COUNT, sipPy);
        %End
    };

    //int_vect_t extint_vector[EXTINT_PIN_COUNT];
    SIP_PYBUFFER extint_vector {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->extint_vector, "h", EXTINT_PIN_COUNT, 0);
        %End
        %SetCode
            sipErr = import_from_fixedlen_sequence(sipAPI_arch_avr, sipCpp->extint_vector, "h", EXTINT_PIN_COUNT, sipPy);
        %End
    };

    //int_vect_t pcint_vector[PCINT_BANK_COUNT];
    SIP_PYBUFFER pcint_vector {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->pcint_vector, "h", PCINT_BANK_COUNT, 0);
        %End
        %SetCode
            sipErr = import_from_fixedlen_sequence(sipAPI_arch_avr, sipCpp->pcint_vector, "h", PCINT_BANK_COUNT, sipPy);
        %End
    };

};


class ArchAVR_ExtInt : public Peripheral,
                       public InterruptHandler,
                       public SignalHook
                       /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_extint.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_ExtInt       /PyName=ExtInt/,
        Signal_PinChange    /PyName=PinChange/,
    };

    ArchAVR_ExtInt(const ArchAVR_ExtIntConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void interrupt_ack_handler(int_vect_t);
    virtual void raised(const signal_data_t&, int);

};
