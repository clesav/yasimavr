/*
 * arch_xt_misc.h
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

#ifndef __YASIMAVR_XT_MISC_H__
#define __YASIMAVR_XT_MISC_H__

#include "arch_xt_globals.h"
#include "core/sim_interrupt.h"
#include "core/sim_types.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * Implementation of a Voltage Reference controller for XT core series
 */

struct ArchXT_VREFConfig {

    struct reference_config_t : base_reg_config_t {
        VREF::Source source;
        double level;
    };

    struct channel_t {
        regbit_t rb_select;
        std::vector<reference_config_t> references;
    };

    std::vector<channel_t> channels;

    reg_addr_t reg_base;

};

class AVR_ARCHXT_PUBLIC_API ArchXT_VREF : public VREF {

public:

    explicit ArchXT_VREF(const ArchXT_VREFConfig& config);

    virtual bool init(Device&) override;
    virtual void reset() override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const ArchXT_VREFConfig& m_config;

    void set_channel_reference(unsigned int index, uint8_t reg_value);

};


//=======================================================================================
/*
 * Implementation of a CPU Interrupt controller for XT core series
 * Unsupported features:
 *      - Round-robin scheduling
 *      - Compact vector table
 *      - Interrupt Vector Select feature
 */

struct ArchXT_IntCtrlConfig {

    unsigned int vector_count;
    reg_addr_t reg_base;

};


class AVR_ARCHXT_PUBLIC_API ArchXT_IntCtrl : public InterruptController {

public:

    explicit ArchXT_IntCtrl(const ArchXT_IntCtrlConfig& config);

    virtual bool init(Device& device) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void cpu_reti() override;

protected:

    virtual void cpu_ack_irq(int_vect_t vector) override;
    virtual int_vect_t get_next_irq() const override;

private:

    const ArchXT_IntCtrlConfig& m_config;

};


//=======================================================================================
/*
 * Implementation of a Reset controller for XT core series
 * Supported features :
 *  - Reset flag (register RSTFR)
 *  - Software reset (register SWRR)
 */

class AVR_ARCHXT_PUBLIC_API ArchXT_ResetCtrl : public Peripheral {

public:

    ArchXT_ResetCtrl(reg_addr_t base);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const reg_addr_t m_base_reg;
    uint8_t m_rst_flags;

};


//=======================================================================================
/*
 *
 */

#define MCU_REVID                       0xFF

#define AVR_CTLREQ_WRITE_SIGROW         1


struct ArchXT_MiscConfig {

    reg_addr_t reg_base_gpior;
    unsigned int gpior_count;

    reg_addr_t reg_revid;

    reg_addr_t reg_base_sigrow;
    uint32_t dev_id;

};


class AVR_ARCHXT_PUBLIC_API ArchXT_MiscRegCtrl : public Peripheral {

public:

    ArchXT_MiscRegCtrl(const ArchXT_MiscConfig& config);
    virtual ~ArchXT_MiscRegCtrl();

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const ArchXT_MiscConfig& m_config;
    uint8_t* m_sigrow;

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_MISC_H__
