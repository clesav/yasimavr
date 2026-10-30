/*
 * arch_avr_twi.h
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_AVR_TWI_H__
#define __YASIMAVR_AVR_TWI_H__

#include "arch_avr_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \brief Configuration structure for ArchAVR_TWI
 */
struct ArchAVR_TWIConfig {

    /// Array of clock prescaler settings
    std::vector<unsigned long> ps_factors;
    /// Address for the control register (TWCR)
    reg_addr_t    reg_ctrl;
    /// Bitspec for the TWEN register bit
    bitspec_t     bs_enable;
    /// Bitspec for the TWSTA register bit
    bitspec_t     bs_start;
    /// Bitspec for the TWSTO register bit
    bitspec_t     bs_stop;
    /// Bitspec for the TWIE register bit
    bitspec_t     bs_int_enable;
    /// Bitspec for the TWINT register bit
    bitspec_t     bs_int_flag;
    /// Bitspec for the TWEA register bit
    bitspec_t     bs_ack_enable;
    /// Address for the bitrate register (TWBR)
    reg_addr_t  reg_bitrate;
    /// Regbit for the status register field
    regbit_t    rb_status;
    /// Regbit for the prescaler register field
    regbit_t    rb_prescaler;
    /// Address for the data register (TWDR)
    reg_addr_t  reg_data;
    /// Regbit for the bus address register field
    regbit_t    rb_addr;
    /// Regbit for the general call register bits
    regbit_t    rb_gencall_enable;
    /// Regbit for the bus address mask register field
    regbit_t    rb_addr_mask;
    /// Index of the interrupt vector
    int_vect_t  iv_twi;

};

/**
   \brief Implementation of a %TWI model for the AVR series

   __CTLREQs supported__ :
   - AVR_CTLREQ_GET_SIGNAL: returns the signal of the underlying %TWI interface - for debug purpose
     - data.index:
       - if =0, returns the signal of the host side.
       - if !=0, returns the signal of the client side
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_TWI : public Peripheral {

public:

    ArchAVR_TWI(uint8_t num, const ArchAVR_TWIConfig& config);
    virtual ~ArchAVR_TWI();

    virtual bool init(Device& device) override;
    virtual void reset(int flags) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t *data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    class _Client;
    friend class _Client;
    class _Host;
    friend class _Host;
    class _PinDriver;
    friend class _PinDriver;

    const ArchAVR_TWIConfig& m_config;

    _Client* m_client;
    _Host* m_host;
    _PinDriver* m_driver;
    BoundFunctionSignalHook<ArchAVR_TWI> m_host_hook;
    BoundFunctionSignalHook<ArchAVR_TWI> m_client_hook;
    bool m_gencall;
    bool m_latched_ack;

    InterruptFlag m_intflag;

    void host_signal_raised(const signal_data_t& sigdata, int hooktag);
    void client_signal_raised(const signal_data_t& sigdata, int hooktag);
    void execute_command(bool sta, bool sto);
    void raise_flag_and_status(uint8_t status);
    void clear_flag_and_status();
    bool address_match(uint8_t addr_rw);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_TWI_H__
