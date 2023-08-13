/*
 * adc.sip
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

#include "ioctrl_common/sim_adc.h"

const ctlreq_id_t CTLREQ_ADC_SET_TEMP = AVR_CTLREQ_ADC_SET_TEMP;
const ctlreq_id_t CTLREQ_ADC_TRIGGER = AVR_CTLREQ_ADC_TRIGGER;

%End

const ctlreq_id_t CTLREQ_ADC_SET_TEMP;
const ctlreq_id_t CTLREQ_ADC_TRIGGER;


//=======================================================================================

class ADC /Abstract/ {
%TypeHeaderCode
#include "ioctrl_common/sim_adc.h"
%End

public:

    enum Channel {
        Channel_SingleEnded     /PyName=SingleEnded/,
        Channel_Differential    /PyName=Differential/,
        Channel_Zero            /PyName=Zero/,
        Channel_IntRef          /PyName=IntRef/,
        Channel_Temperature     /PyName=Temperature/,
        Channel_AcompRef        /PyName=AcompRef/,
    };


    struct channel_config_t : base_reg_config_t {

        ADC::Channel type;
        pin_id_t pin_p;
        pin_id_t pin_n;
        char per_num;
        unsigned int gain;

    };


    enum SignalId /BaseType=IntEnum/ {
        Signal_ConversionStarted    /PyName=ConversionStarted/,
        Signal_AboutToSample        /PyName=AboutToSample/,
        Signal_ConversionComplete   /PyName=ConversionComplete/,
    };

};
