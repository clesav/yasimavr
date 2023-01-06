/*
 * arch_avr_timer.sip
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

struct AVR_ArchAVR_TimerConfig {
%TypeHeaderCode
#include "arch_avr_timer.h"
%End

    enum ClockSource {
        ClockDisabled,
        INT_IO_CLOCK,
    };

    struct clock_config_t : base_reg_config_t {
        AVR_ArchAVR_TimerConfig::ClockSource source;
        uint32_t div;
    };

    enum Mode {
        MODE_INVALID,
        MODE_NORMAL,
        MODE_CTC,
    };

    struct mode_config_t : base_reg_config_t {
        AVR_ArchAVR_TimerConfig::Mode mode;
    };

    struct int_config_t {
        int_vect_t vector;
        uint8_t bit;
    };

    bool is_16bits;
    std::vector<AVR_ArchAVR_TimerConfig::clock_config_t> clocks;
    std::vector<AVR_ArchAVR_TimerConfig::mode_config_t> modes;
    regbit_t rb_clock;
    regbit_t rb_mode;
    regbit_t rb_mode_ext;
    reg_addr_t reg_cnt;
    reg_addr_t reg_ocra;
    reg_addr_t reg_ocrb;
    reg_addr_t reg_int_enable;
    reg_addr_t reg_int_flag;
    AVR_ArchAVR_TimerConfig::int_config_t int_ovf;
    AVR_ArchAVR_TimerConfig::int_config_t int_ocra;
    AVR_ArchAVR_TimerConfig::int_config_t int_ocrb;
};


class AVR_ArchAVR_Timer : public AVR_Peripheral,
                          public AVR_SignalHook
                          /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_timer.h"
%End

public:

    enum Signal /BaseType=IntEnum/ {
        Signal_CompA,
        Signal_CompB,
        Signal_OVF
    };

    AVR_ArchAVR_Timer(int, const AVR_ArchAVR_TimerConfig&);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, uint16_t);

};
