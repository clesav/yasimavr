/*
 * arch_avr_clock.h
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

#ifndef __YASIMAVR_AVR_CLOCK_H__
#define __YASIMAVR_AVR_CLOCK_H__

#include "arch_avr_globals.h"
#include "core/sim_peripheral.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \brief Configuration structure for ArchAVR_ClkCtrl.
 */
struct ArchAVR_ClkCtrlConfig {

    struct clk_config_t {
        sim_id_t id;
        bool ext;
        double freq;
        uint8_t sel_lo;
        uint8_t sel_hi;
    };

    std::vector<clk_config_t> clk_configs;

    regbit_t rb_clkps;

};


/**
   \brief Implementation of an ADC for AVR series
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_ClkCtrl : public Peripheral {

public:

    ArchAVR_ClkCtrl(const ArchAVR_ClkCtrlConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset(int flags) override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;

private:

    const ArchAVR_ClkCtrlConfig& m_config;

    bool read_fuse(int fuse, uint8_t* value);
    void update_clocks();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_CLOCK_H__
