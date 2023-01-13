/*
 * arch_mega0_misc.sip
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

%ModuleCode

#include "arch_mega0_misc.h"

const uint16_t CTLREQ_WRITE_SIGROW = AVR_CTLREQ_WRITE_SIGROW;

%End

const uint16_t CTLREQ_WRITE_SIGROW;


//=======================================================================================

struct AVR_ArchMega0_VREF_Config {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

    struct reference_config_t : base_reg_config_t {
        AVR_IO_VREF::Source source;
        double level;
    };

    struct channel_t {
        regbit_t rb_select;
        std::vector<AVR_ArchMega0_VREF_Config::reference_config_t> references;
    };

    std::vector<AVR_ArchMega0_VREF_Config::channel_t> channels;

    reg_addr_t reg_base;

};

class AVR_ArchMega0_VREF : public AVR_IO_VREF /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

public:

    AVR_ArchMega0_VREF(const AVR_ArchMega0_VREF_Config& /KeepReference/);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};


//=======================================================================================

struct AVR_ArchMega0_IntCtrl_Config {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

    unsigned int vector_count;
    reg_addr_t reg_base;

};

class AVR_ArchMega0_IntCtrl : public AVR_InterruptController /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

public:

    AVR_ArchMega0_IntCtrl(const AVR_ArchMega0_IntCtrl_Config& /KeepReference/);

    virtual bool init(AVR_Device&);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void cpu_reti();

protected:

    virtual void cpu_ack_irq(int_vect_t);
    virtual int_vect_t get_next_irq() const;

};


//=======================================================================================

class AVR_ArchMega0_ResetCtrl : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

public:

    AVR_ArchMega0_ResetCtrl(reg_addr_t);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};


//=======================================================================================

struct AVR_ArchMega0_Misc_Config {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

    reg_addr_t reg_base_gpior;
    unsigned int gpior_count;
    reg_addr_t reg_revid;
    reg_addr_t reg_base_sigrow;
    uint32_t dev_id;

};

class AVR_ArchMega0_MiscRegCtrl : public AVR_Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_mega0_misc.h"
%End

public:

    AVR_ArchMega0_MiscRegCtrl(const AVR_ArchMega0_Misc_Config& /KeepReference/);

    virtual bool init(AVR_Device&);
    virtual void reset();
    virtual bool ctlreq(uint16_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};
