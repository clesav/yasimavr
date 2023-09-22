/*
 * arch_xt_twi.h
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

#ifndef __YASIMAVR_XT_TWI_H__
#define __YASIMAVR_XT_TWI_H__

#include "arch_xt_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_twi.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_twi
   \brief Configuration structure for ArchXT_TWI
 */
struct ArchXT_TWIConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for the master part
    int_vect_t iv_master;
    /// Interrupt vector index for the slave part
    int_vect_t iv_slave;

};

/**
   \ingroup api_twi
   \brief Implementation of a Two Wire Interface for XT core series

   Unsupported features:
    - SDA Setup time
    - Dual mode control
    - DBGRUN
    - Bus timeout
    - write collision flag not supported
    - SMBus compatibility not supported
    - Fast mode not supported

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL : returns a pointer to the TWI signal.
    - AVR_CTLREQ_TWI_ENDPOINT : returns a pointer to the bus end point.

   \sa TWI, TWIEndPoint
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_TWI : public Peripheral, public SignalHook {

public:

    ArchXT_TWI(uint8_t num, const ArchXT_TWIConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchXT_TWIConfig& m_config;

    TWI m_twi;
    bool m_has_address;
    bool m_has_master_rx_data;
    bool m_has_slave_rx_data;

    InterruptFlag m_intflag_master;
    InterruptFlag m_intflag_slave;

    void set_master_enabled(bool enabled);
    void clear_master_status();
    void clear_slave_status();
    bool address_match(uint8_t address);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TWI_H__
