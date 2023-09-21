/*
 * arch_avr_misc.cpp
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

#include "arch_avr_misc.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_VREF::ArchAVR_VREF(double band_gap)
:VREF(1)
{
    set_reference(0, Source_Internal, band_gap);
}


//=======================================================================================

ArchAVR_IntCtrl::ArchAVR_IntCtrl(unsigned int size)
:InterruptController(size)
{}

/**
   Implementation of the interrupt arbiration as per the AVR series.
   The lowest vectors have higher priority.
 */
int_vect_t ArchAVR_IntCtrl::get_next_irq() const
{
    for (int_vect_t i = 0; i < intr_count(); ++i) {
        if (interrupt_raised(i))
            return i;
    }
    return AVR_INTERRUPT_NONE;
}


//=======================================================================================

ArchAVR_MiscRegCtrl::ArchAVR_MiscRegCtrl(const ArchAVR_MiscConfig& config)
:Peripheral(chr_to_id('M', 'I', 'S', 'C'))
,m_config(config)
{}

bool ArchAVR_MiscRegCtrl::init(Device& device)
{
    bool status = Peripheral::init(device);

    for (uint16_t r : m_config.gpior)
        add_ioreg(r);

    return status;
}
