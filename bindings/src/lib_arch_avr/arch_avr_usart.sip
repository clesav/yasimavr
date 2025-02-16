/*
 * arch_avr_usart.sip
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

struct ArchAVR_USARTConfig {
%TypeHeaderCode
#include "arch_avr_usart.h"
%End

    regbit_compound_t rbc_rx_data;
    regbit_compound_t rbc_tx_data;

    regbit_t rb_rx_enable;
    regbit_t rb_tx_enable;
    regbit_t rb_rxc_inten;
    regbit_t rb_rxc_flag;
    regbit_t rb_txc_inten;
    regbit_t rb_txc_flag;
    regbit_t rb_txe_inten;
    regbit_t rb_txe_flag;

    regbit_t rb_baud_2x;
    regbit_compound_t rbc_baud;

    regbit_t rb_ferr;
    regbit_t rb_overrun;
    regbit_t rb_perr;
    regbit_compound_t rbc_chsize;
    regbit_t rb_clock_mode;
    regbit_t rb_parity;
    regbit_t rb_stopbits;

    int_vect_t iv_rxc;
    int_vect_t iv_txc;
    int_vect_t iv_txe;

};


class ArchAVR_USART : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_usart.h"
%End

public:

    ArchAVR_USART(uint8_t, const ArchAVR_USARTConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};
