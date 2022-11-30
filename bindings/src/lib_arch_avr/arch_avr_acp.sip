/*
 * arch_avr_acp.sip
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

struct AVR_ArchAVR_ACP_Config {
%TypeHeaderCode
#include "arch_avr_acp.h"
%End

    struct mux_config_t : base_reg_config_t {
        uint32_t pin;
    };

    std::vector<AVR_ArchAVR_ACP_Config::mux_config_t> mux_pins;

    uint32_t pos_pin;
    uint32_t neg_pin;

    regbit_t rb_disable;
    regbit_t rb_mux_enable;
    regbit_t rb_adc_enable;
    regbit_t rb_mux;
    regbit_t rb_bandgap_select;
    regbit_t rb_int_mode;
    regbit_t rb_output;
    regbit_t rb_int_enable;
    regbit_t rb_int_flag;

    int_vect_t iv_cmp;

};


class AVR_ArchAVR_ACP : public AVR_IO_ACP,
                        public AVR_Peripheral,
                        public AVR_SignalHook
                        /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_acp.h"
%End

public:

    AVR_ArchAVR_ACP(int num, const AVR_ArchAVR_ACP_Config&);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, uint16_t);

};
