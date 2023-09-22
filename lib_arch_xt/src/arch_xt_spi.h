/*
 * arch_xt_spi.h
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

#ifndef __YASIMAVR_XT_SPI_H__
#define __YASIMAVR_XT_SPI_H__

#include "arch_xt_globals.h"
#include "ioctrl_common/sim_spi.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_spi
   \brief Configuration structure for ArchXT_SPI.
 */
struct ArchXT_SPIConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Identifier of the pin used for Chip Select
    pin_id_t pin_select;
    /// Interrupt vector index
    int_vect_t iv_spi;

};

/**
   \ingroup api_spi
   \brief Implementation of a Serial Peripheral Interface controller for the XT core series

   Features:
    - Host/client mode
    - data order, phase and polarity settings have no effect
    - write collision flag not supported
    - buffer mode not supported
    - multi-host mode not supported
    - Slave Select has no effect in host mode (the SSD setting has no effect)

    for supported CTLREQs, see sim_spi.h
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_SPI : public Peripheral, public SignalHook {

public:

    ArchXT_SPI(int num, const ArchXT_SPIConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchXT_SPIConfig& m_config;

    SPI m_spi;

    Pin* m_pin_select;
    bool m_pin_selected;

    InterruptFlag m_intflag;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_SPI_H__
