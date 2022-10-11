/*
 * arch_mega0_twi.h
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

#ifndef __YASIMAVR_MEGA0_TWI_H__
#define __YASIMAVR_MEGA0_TWI_H__


#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_twi.h"


//=======================================================================================
/*
 * Implementation of a TWI for the Mega-0/Mega-1 series
 * Features:
 *  - Host/client mode
 *  - data order, phase and polarity settings have no effect
 *  - write collision flag not supported
 *
 *  for supported CTLREQs, see sim_spi.h
 */

struct AVR_ArchMega0_TWI_Config {

    reg_addr_t reg_base;
    int_vect_t iv_master;
    int_vect_t iv_slave;

};

class DLL_EXPORT AVR_ArchMega0_TWI : public AVR_Peripheral, public AVR_SignalHook {

public:

    AVR_ArchMega0_TWI(uint8_t num, const AVR_ArchMega0_TWI_Config& config);

    virtual bool init(AVR_Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_read_handler(reg_addr_t addr) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override;

private:

    const AVR_ArchMega0_TWI_Config& m_config;

    AVR_IO_TWI m_twi;
    bool m_has_address;
    bool m_has_master_rx_data;
    bool m_has_slave_rx_data;

    AVR_InterruptFlag m_intflag_master;
    AVR_InterruptFlag m_intflag_slave;

    void set_master_enabled(bool enabled);
    void clear_master_status();
    void clear_slave_status();
    bool address_match(uint8_t address);

};

#endif //__YASIMAVR_MEGA0_TWI_H__
