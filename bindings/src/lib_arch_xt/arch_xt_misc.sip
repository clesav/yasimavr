/*
 * arch_xt_misc.sip
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

#include "arch_xt_misc.h"

const ctlreq_id_t CTLREQ_WRITE_SIGROW = AVR_CTLREQ_WRITE_SIGROW;

%End


//=======================================================================================

const ctlreq_id_t CTLREQ_WRITE_SIGROW;


struct ArchXT_VREFConfig {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

    struct reference_config_t : base_reg_config_t {
        VREF::Source source;
        double level;
    };

    struct channel_t {
        regbit_t rb_select;
        std::vector<ArchXT_VREFConfig::reference_config_t> references;
    };

    std::vector<ArchXT_VREFConfig::channel_t> channels;

    reg_addr_t reg_base;

};


class ArchXT_VREF : public VREF /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

public:

    ArchXT_VREF(const ArchXT_VREFConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};


struct ArchXT_IntCtrlConfig {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

    unsigned int vector_count;
    unsigned int vector_size;
    reg_addr_t reg_base;

};


class ArchXT_IntCtrl : public InterruptController /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

public:

    ArchXT_IntCtrl(const ArchXT_IntCtrlConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void cpu_reti();

protected:

    virtual void cpu_ack_irq(int_vect_t);
    virtual InterruptController::IRQ_t get_next_irq() const /Factory/;

};


class ArchXT_ResetCtrl : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

public:

    ArchXT_ResetCtrl(reg_addr_t);

    virtual bool init(Device&);
    virtual void reset();
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};


struct ArchXT_MiscConfig {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

    reg_addr_t reg_base_gpior;
    unsigned int gpior_count;
    reg_addr_t reg_revid;
    reg_addr_t reg_base_sigrow;
    uint32_t dev_id;

};


class ArchXT_MiscRegCtrl : public Peripheral /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_xt_misc.h"
%End

public:

    ArchXT_MiscRegCtrl(const ArchXT_MiscConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);

};
