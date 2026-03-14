/*
 * core.sip
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

enum SREG_Bits {
    SREG_C,
    SREG_Z,
    SREG_N,
    SREG_V,
    SREG_S,
    SREG_H,
    SREG_T,
    SREG_I,
};


enum StdRegisters {
    R_XL, R_XH,
    R_YL, R_YH,
    R_ZL, R_ZH,
    R_X,
    R_Y,
    R_Z,
    R_SPL,
    R_SPH,
    R_SREG,
};


struct breakpoint_t {
%TypeHeaderCode
#include "core/sim_core.h"
%End

    flash_addr_t addr;

    //uint8_t instr[4];
    SIP_PYBUFFER instr /NoSetter/ {
        %GetCode
            sipPy = sipConvertToArray(sipCpp->instr, "B", 4, 0);
        %End
    };

    uint8_t instr_len;

};


class Core /Abstract/ {
%TypeHeaderCode
#include "core/sim_core.h"
%End

public:

    enum NVM /BaseType=IntEnum/ {
        NVM_Flash           /PyName=Flash/,
        NVM_Fuses           /PyName=Fuses/,
        NVM_CommonCount     /PyName=CommonCount/,
        NVM_ArchDefined     /PyName=ArchDefined/,
        NVM_GetCount        /PyName=GetCount/,
    };

    Core(const CoreConfiguration&);

};
