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

#include "arch_avr_globals.h"
#include "core/sim_interrupt.h"
#include "core/sim_memory.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \ingroup api_vref
   \brief Implementation of a Voltage Reference controller for AVR series

   It supports only one fixed reference.
 */

class AVR_ARCHAVR_PUBLIC_API ArchAVR_VREF : public VREF {

public:

    explicit ArchAVR_VREF(double band_gap);

};


//=======================================================================================
/**
   \brief Implementation of a interrupt controller for AVR series
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_IntCtrl : public InterruptController, SignalHook {

public:

    ArchAVR_IntCtrl(unsigned int vector_count, unsigned int vector_size);

    virtual bool init(Device& device) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

protected:

    virtual IRQ_t get_next_irq() const override;

private:

    unsigned int m_vector_size;
    MemorySectionManager* m_sections;

};


//=======================================================================================
/**
   \brief Configuration structure for ArchAVR_MiscRegCtrl
 */
struct ArchAVR_MiscConfig {

    /// Array of addresses for the GPIORx registers
	std::vector<reg_addr_t> gpior;

};

/**
   \brief Implementation of a misc controller for AVR series
   
   This implementation supports the general purpose registers GPIORx.
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_MiscRegCtrl : public Peripheral {

public:

    explicit ArchAVR_MiscRegCtrl(const ArchAVR_MiscConfig& config);

    virtual bool init(Device& device) override;

private:

    const ArchAVR_MiscConfig& m_config;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_MISC_H__
