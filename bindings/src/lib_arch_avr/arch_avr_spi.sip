/*
 * arch_avr_spi.sip
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

struct ArchAVR_SPIConfig {
%TypeHeaderCode
#include "arch_avr_spi.h"
%End

    reg_addr_t reg_data;

    regbit_t rb_enable;
    regbit_t rb_int_enable;
    regbit_t rb_int_flag;
    regbit_t rb_mode;

    pin_id_t pin_select;

    regbit_t rb_clock;
    regbit_t rb_clock2x;

    int_vect_t iv_spi;

};


class ArchAVR_SPI : public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_spi.h"
%End

public:

    ArchAVR_SPI(uint8_t, const ArchAVR_SPIConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, int);

};
