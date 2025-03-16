/*
 * arch_avr_extint.sip
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

struct ArchAVR_ExtIntConfig {
%TypeHeaderCode
#include "arch_avr_extint.h"
#include "utils/buffer_utils.h"
%End

    struct ext_int_t {
        int_vect_t vector;
        pin_id_t pin;
    };

    struct pc_int_t {
        int_vect_t vector;
        reg_addr_t reg_mask;

        SIP_PYBUFFER pins {
            %GetCode
                sipPy = sipConvertToArray(sipCpp->pins, "I", 8, 0);
            %End
            %SetCode
                sipErr = import_from_fixedlen_sequence(sipAPI_arch_avr, sipCpp->pins, "I", 8, sipPy);
            %End
        };
    };

    std::vector<ArchAVR_ExtIntConfig::ext_int_t> ext_ints;
    std::vector<ArchAVR_ExtIntConfig::pc_int_t> pc_ints;
    regbit_t rb_extint_ctrl;
    regbit_t rb_extint_mask;
    regbit_t rb_extint_flag;
    regbit_t rb_pcint_ctrl;
    regbit_t rb_pcint_flag;
};


class ArchAVR_ExtInt : public Peripheral,
                       public InterruptHandler
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

};
