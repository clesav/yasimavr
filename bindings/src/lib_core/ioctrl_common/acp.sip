/*
 * acp.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

class ACP /Abstract/ {
%TypeHeaderCode
#include "ioctrl_common/sim_acp.h"
%End

public:

    enum Channel {
        Channel_Pin         /PyName=Pin/,
        Channel_AcompRef    /PyName=AcompRef/,
    };

     struct channel_config_t : base_reg_config_t {
        ACP::Channel type;
        pin_id_t pin;
    };

    enum SignalId /BaseType=IntEnum/ {
        Signal_Output       /PyName=Output/,
        Signal_DAC          /PyName=DAC/,
    };

};
