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


//=======================================================================================
/*
 * Implementation of a Voltage Reference controller for AVR series
 */

class DLL_EXPORT AVR_ArchAVR_VREF : public AVR_IO_VREF {

public:

    AVR_ArchAVR_VREF(double band_gap);

protected:

    virtual double get_reference(User user) const override;

private:

    double m_band_gap;

};


//=======================================================================================
/*
 * Implementation of a interrupt controller for AVR series
 */
class DLL_EXPORT AVR_ArchAVR_Interrupt : public AVR_InterruptController {

public:

    AVR_ArchAVR_Interrupt(unsigned int size);

protected:

    virtual int_vect_t get_next_irq() const override;

};


//=======================================================================================
/*
 * Implementation of a interrupt controller for AVR series
 */

struct AVR_ArchAVR_Misc_Config {

    std::vector<reg_addr_t> gpior;

};


class DLL_EXPORT AVR_ArchAVR_MiscRegCtrl : public AVR_Peripheral {

public:

    AVR_ArchAVR_MiscRegCtrl(const AVR_ArchAVR_Misc_Config& config);

    virtual bool init(AVR_Device& device) override;

private:

    const AVR_ArchAVR_Misc_Config& m_config;

};

#endif //__YASIMAVR_AVR_MISC_H__
