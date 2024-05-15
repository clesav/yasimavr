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
#include "arch_avr_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_VREF::ArchAVR_VREF(double band_gap)
:VREF(1)
{
    set_reference(0, Source_Internal, band_gap);
}


//=======================================================================================

ArchAVR_IntCtrl::ArchAVR_IntCtrl(unsigned int vector_count, unsigned int vector_size)
:InterruptController(vector_count)
,m_vector_size(vector_size)
,m_sections(nullptr)
{}


bool ArchAVR_IntCtrl::init(Device& device)
{
    bool status = InterruptController::init(device);

    //Obtain the pointer to the flash section manager
    ctlreq_data_t req;
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SECTIONS, &req))
        return false;
    m_sections = reinterpret_cast<MemorySectionManager*>(req.data.as_ptr());

    if (m_sections)
        m_sections->signal().connect(*this);

    return status;
}

/**
   Implementation of the interrupt arbitration as per the AVR series.
   The lowest vectors have higher priority.
 */
InterruptController::IRQ_t ArchAVR_IntCtrl::get_next_irq() const
{
    //Check if interrupts are disabled for the current section of flash
    if (m_sections->access_flags(m_sections->current_section()) & ArchAVR_Device::Access_IntDisabled)
        return InterruptController::NO_INTERRUPT;

    for (int_vect_t i = 0; i < intr_count(); ++i) {
        if (interrupt_raised(i))
            return { i, i * m_vector_size, false };
    }

    return InterruptController::NO_INTERRUPT;
}

/**
   Implementation of the signal hook. On a section change, the IRQs need to updated to take into account
   the access flag IntDisabled.
 */
void ArchAVR_IntCtrl::raised(const signal_data_t& sigdata, int)
{
    if (sigdata.sigid == MemorySectionManager::Signal_Enter)
        update_irq();
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
