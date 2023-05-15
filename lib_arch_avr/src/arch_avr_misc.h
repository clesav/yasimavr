/*
 * arch_avr_misc.h
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

#ifndef __YASIMAVR_AVR_MISC_H__
#define __YASIMAVR_AVR_MISC_H__

#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Implementation of a Voltage Reference controller for AVR series
 */

class DLL_EXPORT ArchAVR_VREF : public VREF {

public:

    explicit ArchAVR_VREF(double band_gap);

};


//=======================================================================================
/*
 * Implementation of a interrupt controller for AVR series
 */
class DLL_EXPORT ArchAVR_IntCtrl : public InterruptController {

public:

    explicit ArchAVR_IntCtrl(unsigned int size);

protected:

    virtual int_vect_t get_next_irq() const override;

};


//=======================================================================================
/*
 * Implementation of a interrupt controller for AVR series
 */

struct ArchAVR_Misc_Config {

    std::vector<reg_addr_t> gpior;

};


class DLL_EXPORT ArchAVR_MiscRegCtrl : public Peripheral {

public:

    explicit ArchAVR_MiscRegCtrl(const ArchAVR_Misc_Config& config);

    virtual bool init(Device& device) override;

private:

    const ArchAVR_Misc_Config& m_config;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_MISC_H__
