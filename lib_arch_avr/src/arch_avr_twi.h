/*
 * arch_avr_twi.h
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

#ifndef __YASIMAVR_AVR_TWI_H__
#define __YASIMAVR_AVR_TWI_H__

#include "arch_avr_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_twi.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \ingroup api_twi
   \brief Configuration structure for ArchAVR_TWI
 */
struct ArchAVR_TWIConfig {

    std::vector<unsigned long> ps_factors;

    reg_addr_t  reg_ctrl;
    bitmask_t   bm_enable;
    bitmask_t   bm_start;
    bitmask_t   bm_stop;
    bitmask_t   bm_int_enable;
    bitmask_t   bm_int_flag;
    bitmask_t   bm_ack_enable;

    reg_addr_t  reg_bitrate;
    regbit_t    rb_status;
    regbit_t    rb_prescaler;
    reg_addr_t  reg_data;
    regbit_t    rb_addr;
    regbit_t    rb_gencall_enable;
    regbit_t    rb_addr_mask;
    int_vect_t  iv_twi;

};

/**
   \ingroup api_twi
   \brief Implementation of a TWI model for the AVR series

   Features:
    - Host/client mode
    - data order, phase and polarity settings have no effect
    - write collision flag not supported

    \sa sim_twi.h
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_TWI : public Peripheral, public SignalHook {

public:

    ArchAVR_TWI(uint8_t num, const ArchAVR_TWIConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t *data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchAVR_TWIConfig& m_config;

    TWI m_twi;
    bool m_gencall;
    bool m_rx;

    InterruptFlag m_intflag;

    void set_intflag(uint8_t status);
    void clear_intflag();
    bool address_match(uint8_t address);

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_TWI_H__
