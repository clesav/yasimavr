/*
 * vref.sip
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

#include "ioctrl_common/sim_vref.h"

const ctlreq_id_t CTLREQ_VREF_GET = AVR_CTLREQ_VREF_GET;
const ctlreq_id_t CTLREQ_VREF_SET_REF = AVR_CTLREQ_VREF_SET_REF;
const ctlreq_id_t CTLREQ_VREF_SET_MUX = AVR_CTLREQ_VREF_SET_MUX;
const ctlreq_id_t CTLREQ_VREF_SET_TEMP = AVR_CTLREQ_VREF_SET_TEMP;

%End


//=======================================================================================

const ctlreq_id_t CTLREQ_VREF_GET;
const ctlreq_id_t CTLREQ_VREF_SET_REF;
const ctlreq_id_t CTLREQ_VREF_SET_MUX;
const ctlreq_id_t CTLREQ_VREF_SET_TEMP;


struct VREFConfig {
%TypeHeaderCode
#include "ioctrl_common/sim_vref.h"
%End

    double temp_cal_25C;
    double temp_cal_coef;

};


class VREF : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_vref.h"
%End

public:

    enum Source /BaseType=IntEnum/ {
        Source_VCC                  /PyName=VCC/,
        Source_AVCC                 /PyName=AVCC/,
        Source_AREF                 /PyName=AREF/,
        Source_Bandgap              /PyName=Bandgap/,
        Source_Temperature          /PyName=Temperature/,
        Source_Mux                  /PyName=Mux/,
    };

    enum SignalId /BaseType=IntEnum/ {
        Signal_ARefChange           /PyName=ARefChange/,
        Signal_IntRefChange         /PyName=IntRefChange/,
        Signal_VCCChange            /PyName=VCCChange/,
        Signal_TempChange           /PyName=TempChange/,
    };

    struct getset_t {

        Source source;
        unsigned int channel;
        double voltage;

        getset_t(Source, unsigned int=0, double=-1.0);
        %MethodCode
            sipCpp = new VREF::getset_t();
            *sipCpp = { a0, a1, a2 };
        %End

    };

    VREF(const VREFConfig&, unsigned int);

protected:

    void set_reference(unsigned int, Source, double);
    void set_reference(unsigned int, Source);
    double reference(unsigned int) const;

};
