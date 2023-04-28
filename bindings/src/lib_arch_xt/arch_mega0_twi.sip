/*
 * arch_mega0_twi.sip
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

struct AVR_ArchMega0_TWI_Config {
%TypeHeaderCode
#include "arch_mega0_twi.h"
%End

    reg_addr_t reg_base;
    int_vect_t iv_master;
    int_vect_t iv_slave;

};


class AVR_ArchMega0_TWI : public AVR_Peripheral, public AVR_SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_twi.h"
%End

public:

    AVR_ArchMega0_TWI(uint8_t, const AVR_ArchMega0_TWI_Config& /KeepReference/);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, uint16_t);

};
