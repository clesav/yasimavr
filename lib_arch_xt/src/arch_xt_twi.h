/*
 * arch_xt_twi.h
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

#ifndef __YASIMAVR_XT_TWI_H__
#define __YASIMAVR_XT_TWI_H__

#include "arch_xt_globals.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_twi
   \brief Configuration structure for ArchXT_TWI
 */
struct ArchXT_TWIConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for the host side
    int_vect_t iv_host;
    /// Interrupt vector index for the client side
    int_vect_t iv_client;
    /// Enable the dual port control
    bool dual_ctrl;

};

/**
   \ingroup api_twi
   \brief Implementation of a Two Wire Interface for XT core series

   Unsupported features:
    - SDA Setup time
    - DBGRUN
    - Bus timeout
    - SMBus compatibility
    - Fast mode
    - Quick Command

   CTLREQs supported:
    - AVR_CTLREQ_TWI_BUS_ERROR : Trigger a bus error
            data.index : if =0, trigger a bus error in the host and client sides, if enabled
                         if !=0, trigger a bus error for the client side only, if enabled
    - AVR_CTLREQ_GET_SIGNAL: returns the signal of the underlying TWI interface; for debug purpose
            data.index : if =0, returns the signal of the host side
                         if !=0, returns the signal of the client side

   \sa TWI, TWIEndPoint
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_TWI : public Peripheral {

public:

    ArchXT_TWI(uint8_t num, const ArchXT_TWIConfig& config);
    virtual ~ArchXT_TWI();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual uint8_t ioreg_peek_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    class _Client;
    friend class _Client;
    class _Host;
    friend class _Host;
    class _PinDriver;
    friend class _PinDriver;

    const ArchXT_TWIConfig& m_config;

    _Client* m_client;
    _Host* m_host;
    _PinDriver* m_driver;
    BoundFunctionSignalHook<ArchXT_TWI> m_host_hook;
    BoundFunctionSignalHook<ArchXT_TWI> m_client_hook;
    bool m_pending_host_address;
    bool m_pending_host_rx_data;
    bool m_pending_client_rx_data;
    uint8_t m_host_cmd;
    uint8_t m_client_cmd;

    InterruptFlag m_intflag_host;
    InterruptFlag m_intflag_client;

    void reset_host();
    void clear_host_status();
    void clear_client_status();
    bool address_match(uint8_t addr_byte);
    void execute_host_command();
    void execute_client_command();
    void host_signal_raised(const signal_data_t& sigdata, int);
    void client_signal_raised(const signal_data_t& sigdata, int);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_TWI_H__
