/*
 * arch_avr_twi.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

struct AVR_ArchAVR_TWI_Config {
%TypeHeaderCode
#include "arch_avr_twi.h"
%End

    UnsignedInt32Vector ps_factors;

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


class AVR_ArchAVR_TWI : public AVR_Peripheral, public AVR_SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_twi.h"
%End

public:

    AVR_ArchAVR_TWI(uint8_t, const AVR_ArchAVR_TWI_Config& /KeepReference/);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, uint16_t);

};
