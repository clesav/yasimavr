/*
 * arch_xt_acp.sip
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
%ModuleCode

#include "arch_xt_acp.h"

const uint16_t CTLREQ_ACP_GET_DAC = AVR_CTLREQ_ACP_GET_DAC;

%End

const uint16_t CTLREQ_ACP_GET_DAC;


//=======================================================================================

struct AVR_ArchXT_ACP_Config {
%TypeHeaderCode
#include "arch_xt_acp.h"
%End

    std::vector<AVR_IO_ACP::channel_config_t> pos_channels;
    std::vector<AVR_IO_ACP::channel_config_t> neg_channels;

    uint32_t vref_channel;
    reg_addr_t reg_base;
    int_vect_t iv_cmp;

};


class AVR_ArchXT_ACP : public AVR_IO_ACP,
                       public AVR_Peripheral,
                       public AVR_SignalHook
                       /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_acp.h"
%End

public:

    AVR_ArchXT_ACP(int, const AVR_ArchXT_ACP_Config& /KeepReference/);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void sleep(bool, AVR_SleepMode);
    virtual void raised(const signal_data_t&, uint16_t);

};
