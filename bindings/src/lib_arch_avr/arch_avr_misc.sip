/*
 * arch_avr_misc.sip
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

class AVR_ArchAVR_VREF : public AVR_IO_VREF /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

public:

    AVR_ArchAVR_VREF(double);

};


//=======================================================================================

class AVR_ArchAVR_Interrupt : public AVR_InterruptController /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

public:

    AVR_ArchAVR_Interrupt(unsigned int);

protected:

    virtual int_vect_t get_next_irq() const;

};


//=======================================================================================

struct AVR_ArchAVR_Misc_Config {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

    UnsignedInt16Vector gpior;

};


class AVR_ArchAVR_MiscRegCtrl : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

public:

    AVR_ArchAVR_MiscRegCtrl(const AVR_ArchAVR_Misc_Config&);

    virtual bool init(AVR_Device& device);

};
