/*
 * arch_avr_misc.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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


//=======================================================================================

AVR_ArchAVR_VREF::AVR_ArchAVR_VREF(double band_gap)
:m_band_gap(band_gap)
{}

double AVR_ArchAVR_VREF::get_reference(User user) const
{
	return m_band_gap;
}


//=======================================================================================

AVR_ArchAVR_Interrupt::AVR_ArchAVR_Interrupt(unsigned int size)
:AVR_InterruptController(size)
{}

int_vect_t AVR_ArchAVR_Interrupt::get_next_irq() const
{
	for (int_vect_t i = 0; i < intr_count(); ++i) {
		if (interrupt_state(i) == IntrState_Raised)
			return i;
	}
	return AVR_INTERRUPT_NONE;
}


//=======================================================================================

AVR_ArchAVR_MiscRegCtrl::AVR_ArchAVR_MiscRegCtrl(const AVR_ArchAVR_Misc_Config& config)
:AVR_Peripheral(AVR_ID('M', 'I', 'S', 'C'))
,m_config(config)
{}

bool AVR_ArchAVR_MiscRegCtrl::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	for (uint16_t r : m_config.gpior)
		add_ioreg(r);

	return status;
}
