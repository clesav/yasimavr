/*
 * arch_avr_clock.cpp
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

#include "arch_avr_clock.h"
#include "arch_avr_nvm.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_ClkCtrl::ArchAVR_ClkCtrl(const ArchAVR_ClkCtrlConfig& config)
:Peripheral(AVR_IOCTL_CLOCK)
,m_config(config)
{}


bool ArchAVR_ClkCtrl::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rb_clkps);

    for (auto& cfg : m_config.clk_configs) {
        device.cycle_manager()->add_clock_source(cfg.id);
        device.cycle_manager()->configure_clock_source(cfg.id, cfg.ext ? 0.0 : cfg.freq);
    }

    return status;
}

void ArchAVR_ClkCtrl::reset(int)
{
    uint8_t fuse_clkdiv8 = 1;
    read_fuse(ArchAVR_Fuses::Fuse_ClkDiv8, &fuse_clkdiv8);
    write_ioreg(m_config.rb_clkps, fuse_clkdiv8 ? 0x00 : 0x03);

    update_clocks();
}


bool ArchAVR_ClkCtrl::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_CLK_SET_EXTCLK) {
        sim_id_t req_id = data->id;
        for (auto& cfg : m_config.clk_configs) {
            if (cfg.id == req_id && cfg.ext) {
                device()->cycle_manager()->configure_clock_source(cfg.id, data->data.as_double());
                update_clocks();
                break;
            }
        }
        return true;
    }

    return false;
}


bool ArchAVR_ClkCtrl::read_fuse(int fuse, uint8_t* value)
{
    ctlreq_data_t reqdata = { .index = fuse };
    bool ok = device()->ctlreq("FUSES", AVR_CTLREQ_FUSE_VALUE, &reqdata);

    if (!ok || reqdata.data.as_int() == -1)
        return false;

    *value = reqdata.data.as_int();
    return true;
}


void ArchAVR_ClkCtrl::update_clocks()
{
    uint8_t fuse_clksel;
    sim_id_t src_id;
    if (read_fuse(ArchAVR_Fuses::Fuse_ClkSel, &fuse_clksel)) {
        for (auto& cfg : m_config.clk_configs) {
            if (fuse_clksel >= cfg.sel_lo && fuse_clksel <= cfg.sel_hi) {
                src_id = cfg.id;
                break;
            }
        }
    }

    unsigned long ps_div = 1 << read_ioreg(m_config.rb_clkps);

    device()->cycle_manager()->configure_clock_domain(CycleManager::ReferenceDomain, src_id, ps_div);
}
