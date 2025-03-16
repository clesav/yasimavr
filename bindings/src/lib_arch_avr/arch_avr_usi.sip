/*
 * arch_avr_usi.sip
 *
 *  Copyright 2025 Clement Savergne <csavergne@yahoo.com>

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

struct ArchAVR_USIConfig {
%TypeHeaderCode
#include "arch_avr_usi.h"
%End

    regbit_t rb_wiremode;
    regbit_t rb_clk_sel;
    regbit_t rb_clk_strobe;
    regbit_t rb_clk_toggle;
    reg_addr_t reg_data;
    reg_addr_t reg_buffer;
    regbit_t rb_counter;
    regbit_t rb_ovf_flag;
    regbit_t rb_ovf_inten;
    regbit_t rb_start_flag;
    regbit_t rb_start_inten;
    regbit_t rb_stop_flag;

    int_vect_t iv_ovf;
    int_vect_t iv_start;

};


class ArchAVR_USI : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_usi.h"
%End

public:

    ArchAVR_USI(const ArchAVR_USIConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};
