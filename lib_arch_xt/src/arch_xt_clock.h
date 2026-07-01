/*
 * arch_xt_clock.h
 *
 *  Copyright 2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_XT_CLK_H__
#define __YASIMAVR_XT_CLK_H__

#include "arch_xt_globals.h"
#include "core/sim_peripheral.h"
#include "core/sim_memory.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Implementation of a clock controller for XT core series.

   Applicable for ATMega 0-series, ATTiny 0,1,2series
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_ClkCtrl : public Peripheral {

public:

    static constexpr sim_id_t ClkSrc_Main = "OSC20M";
    static constexpr sim_id_t ClkSrc_ULP32K = "ULP32K";
    static constexpr sim_id_t ClkSrc_EXT32K = "EXT32K";
    static constexpr sim_id_t ClkSrc_EXTCLK = "EXTCLK";

    explicit ArchXT_ClkCtrl(reg_addr_t reg_base);

    virtual bool init(Device&) override;
    virtual void reset(int flags) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

private:

    const reg_addr_t m_reg_base;
    NonVolatileMemory* m_fuses;
    uint8_t m_active_main_sel;
    double m_ext_cryst_freq;
    double m_ext_clk_freq;

    void update_clocks();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_CLK_H__
