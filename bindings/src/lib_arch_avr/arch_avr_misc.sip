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

class ArchAVR_VREF : public VREF /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

public:

    ArchAVR_VREF(double);

};


class ArchAVR_IntCtrl : public InterruptController /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

public:

    ArchAVR_IntCtrl(unsigned int, unsigned int);

    virtual bool init(Device&);
    virtual void raised(const signal_data_t&, int);

protected:

    virtual InterruptController::IRQ_t get_next_irq() const /Factory/;

};


struct ArchAVR_MiscConfig {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

    std::vector<reg_addr_t> gpior;

};


class ArchAVR_MiscRegCtrl : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_misc.h"
%End

public:

    ArchAVR_MiscRegCtrl(const ArchAVR_MiscConfig& /KeepReference/);

    virtual bool init(Device& device);

};
