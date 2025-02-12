/*
 * arch_xt_spi.h
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

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
    /// Interrupt vector index
    int_vect_t iv_spi;

};


/**
   \ingroup api_spi
   \brief Implementation of a Serial Peripheral Interface controller for the XT core series

    for supported CTLREQs, see sim_spi.h
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_SPI : public Peripheral {

public:

    ArchXT_SPI(int num, const ArchXT_SPIConfig& config);
    virtual ~ArchXT_SPI();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    class _PinDriver;
    class _Controller;
    class _InterruptHandler;

    friend class SPIController;

    const ArchXT_SPIConfig& m_config;

    _Controller* m_ctrl;
    _InterruptHandler* m_intflag;

    void frame_completed();
    void host_selected();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_SPI_H__
