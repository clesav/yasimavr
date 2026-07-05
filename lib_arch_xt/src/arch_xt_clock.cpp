/*
 * arch_xt_clock.cpp
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

#include "arch_xt_clock.h"
#include "arch_xt_device.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include <array>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_reg_base + offsetof(CLKCTRL_t, reg))

#define REG_OFS(reg) \
    reg_addr_t(offsetof(CLKCTRL_t, reg))


struct ps_setting_t : base_reg_config_t {
    unsigned long pdiv;
};

static constexpr auto ps_settings = std::to_array<ps_setting_t>({
        { 0x0, 2 }, { 0x1, 4 },  { 0x2, 8 },  { 0x3, 16 },
        { 0x4, 32 }, { 0x5, 64 }, { 0x8, 6 }, { 0x9, 10 },
        { 0xA, 12 }, { 0xB, 24 }, { 0xC, 48 }
});


ArchXT_ClkCtrl::ArchXT_ClkCtrl(reg_addr_t reg_base)
:Peripheral(AVR_IOCTL_CLOCK)
,m_reg_base(reg_base)
,m_fuses(nullptr)
,m_active_main_sel(0)
,m_ext_cryst_freq(0.0)
,m_ext_clk_freq(0.0)
{}


bool ArchXT_ClkCtrl::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(MCLKCTRLA), CLKCTRL_CLKOUT_bm | CLKCTRL_CLKSEL_gm);
    add_ioreg(REG_ADDR(MCLKCTRLB), CLKCTRL_PDIV_gm | CLKCTRL_PEN_bm);
    add_ioreg(REG_ADDR(MCLKLOCK), CLKCTRL_LOCKEN_bm);
    add_ioreg(REG_ADDR(MCLKSTATUS), CLKCTRL_EXTS_bm | CLKCTRL_XOSC32KS_bm | CLKCTRL_OSC32KS_bm |
                                    CLKCTRL_OSC20MS_bm | CLKCTRL_SOSC_bm, IORegister::RO);
    add_ioreg(REG_ADDR(OSC20MCTRLA), CLKCTRL_RUNSTDBY_bm);
    add_ioreg(REG_ADDR(OSC20MCALIBA), CLKCTRL_CAL20M_gm);
    add_ioreg(REG_ADDR(OSC20MCALIBB), CLKCTRL_TEMPCAL20M_gm);
    add_ioreg(REG_ADDR(OSC20MCALIBB), CLKCTRL_LOCK_bm, IORegister::RO);
    add_ioreg(REG_ADDR(OSC32KCTRLA), CLKCTRL_RUNSTDBY_bm);
    add_ioreg(REG_ADDR(XOSC32KCTRLA), CLKCTRL_CSUT_gm | CLKCTRL_SEL_bm | CLKCTRL_RUNSTDBY_bm |
                                      CLKCTRL_ENABLE_bm);

    //Obtain the pointer to the userrow block in RAM
    ctlreq_data_t req = { .index = ArchXT_Core::NVM_Fuses };
    if (!device.ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_NVM, &req))
        return false;
    m_fuses = req.data.as_ptr<NonVolatileMemory>();

    CycleManager& cm = *device.cycle_manager();
    cm.add_clock_source(ClkSrc_Main);
    cm.add_clock_source(ClkSrc_ULP32K);
    cm.add_clock_source(ClkSrc_EXT32K);
    cm.add_clock_source(ClkSrc_EXTCLK);

    cm.configure_clock_source(ClkSrc_ULP32K, 32768.0);

    return status;
}


void ArchXT_ClkCtrl::reset(int)
{
    m_active_main_sel = 0;

    //Set the default prescaler division to 6
    SET_IOREG(MCLKCTRLB, CLKCTRL_PEN);
    WRITE_IOREG_F_GC(MCLKCTRLB, CLKCTRL_PDIV, CLKCTRL_PDIV_6X_gc);
    //Assume the internal oscillators are always stable
    SET_IOREG(MCLKSTATUS, CLKCTRL_OSC20MS);
    SET_IOREG(MCLKSTATUS, CLKCTRL_OSC32KS);
    //Copy the LOCK bit from the fuse
    uint8_t fuse_osccfg = (*m_fuses)[offsetof(FUSE_t, OSCCFG)];
    WRITE_IOREG_B(MCLKLOCK, CLKCTRL_LOCKEN, EXTRACT_B(fuse_osccfg, FUSE_OSCLOCK));
    WRITE_IOREG_B(OSC20MCALIBB, CLKCTRL_LOCK, EXTRACT_B(fuse_osccfg, FUSE_OSCLOCK));

    update_clocks();
}


bool ArchXT_ClkCtrl::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_CLK_SET_EXTCLK) {
        if (data->id == ClkSrc_EXT32K) {
            m_ext_cryst_freq = data->data.as_double();
            if (m_ext_cryst_freq < 0.0) m_ext_cryst_freq = 0.0;
        }
        else if (data->id == ClkSrc_EXTCLK) {
            m_ext_clk_freq = data->data.as_double();
            if (m_ext_clk_freq < 0.0) m_ext_clk_freq = 0.0;
        }
        update_clocks();
        return true;
    }

    return false;
}


void ArchXT_ClkCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_reg_base;

    if (reg_ofs == REG_OFS(MCLKCTRLA)) {
        //If locked, prevent any change
        if (TEST_IOREG(MCLKLOCK, CLKCTRL_LOCKEN))
            WRITE_IOREG(MCLKCTRLA, data.old);

        //If the current selection is an external source, prevent any change
        if ((data.old & CLKCTRL_CLKSEL1_bm) && ((data.value ^ data.old) & CLKCTRL_CLKSEL_gm))
            WRITE_IOREG_F_GC(MCLKCTRLA, CLKCTRL_CLKSEL, data.old);

        update_clocks();
    }

    else if (reg_ofs == REG_OFS(MCLKCTRLB)) {
        //If locked, prevent any change
        if (TEST_IOREG(MCLKLOCK, CLKCTRL_LOCKEN))
            WRITE_IOREG(MCLKCTRLB, data.old);

        update_clocks();
    }

    else if (reg_ofs == REG_OFS(MCLKLOCK)) {
        //If locked, prevent any change
        if (EXTRACT_B(data.old, CLKCTRL_LOCKEN) && !EXTRACT_B(data.value, CLKCTRL_LOCKEN))
            SET_IOREG(MCLKLOCK, CLKCTRL_LOCKEN);
    }

    else if (reg_ofs == REG_OFS(OSC20MCALIBA) || reg_ofs == REG_OFS(OSC20MCALIBB)) {
        //If locked, cannot unlock
        if (TEST_IOREG(MCLKLOCK, CLKCTRL_LOCKEN))
            write_ioreg(addr, data.old);
    }

    else if (reg_ofs == REG_OFS(XOSC32KCTRLA)) {
        //If ENABLE='1' or XOSC32KS='1', SEL and CSUT are read-only
        if (EXTRACT_B(data.old, CLKCTRL_ENABLE) || TEST_IOREG(MCLKSTATUS, CLKCTRL_XOSC32KS)) {
            WRITE_IOREG_B(XOSC32KCTRLA, CLKCTRL_SEL, EXTRACT_B(data.old, CLKCTRL_SEL));
            WRITE_IOREG_F(XOSC32KCTRLA, CLKCTRL_CSUT, EXTRACT_F(data.old, CLKCTRL_CSUT));
        }

        update_clocks();
    }
}


void ArchXT_ClkCtrl::update_clocks()
{
    logger().dbg("Updating clocks");

    CycleManager* cm = device()->cycle_manager();

    //Get the OSCCFG fuse value and determine the main internal clock frequency (16/20MHz)
    double int_main_freq;
    uint8_t fuse_osccfg = (*m_fuses)[offsetof(FUSE_t, OSCCFG)];
    if (EXTRACT_GC(fuse_osccfg, FUSE_FREQSEL) == FREQSEL_20MHZ_gc)
        int_main_freq = 20000000.0;
    else
        int_main_freq = 16000000.0;
    cm->configure_clock_source(ClkSrc_Main, int_main_freq);
    logger().dbg("  OCS20M : f=%f", int_main_freq);

    //Determine the frequency for the crystal oscillator
    bool ext_cryst_on = TEST_IOREG(XOSC32KCTRLA, CLKCTRL_ENABLE) && m_ext_cryst_freq;
    double ext_cryst_freq = ext_cryst_on ? m_ext_cryst_freq : 0.0;
    cm->configure_clock_source(ClkSrc_EXT32K, ext_cryst_freq);
    logger().dbg("  EXT32K : f=%f", ext_cryst_freq);

    //Determine the frequency for the external clock
    cm->configure_clock_source(ClkSrc_EXTCLK, m_ext_clk_freq);
    logger().dbg("  EXTCLK : f=%f", m_ext_clk_freq);

    //If a switch from internal to external clock is requested by writing CLKSEL, check
    //that the new clock is running before actually switching.
    uint8_t reg_main_sel = READ_IOREG_F_GC(MCLKCTRLA, CLKCTRL_CLKSEL);
    if ((reg_main_sel & CLKCTRL_CLKSEL1_bm) && !(m_active_main_sel & CLKCTRL_CLKSEL1_bm)) {
        bool can_switch = (reg_main_sel == CLKCTRL_CLKSEL_XOSC32K_gc && ext_cryst_on) ||
                          (reg_main_sel == CLKCTRL_CLKSEL_EXTCLK_gc && m_ext_clk_freq);
        if (can_switch) {
            m_active_main_sel = reg_main_sel;
            CLEAR_IOREG(MCLKSTATUS, CLKCTRL_SOSC);
        } else {
            SET_IOREG(MCLKSTATUS, CLKCTRL_SOSC);
        }
    } else {
        m_active_main_sel = reg_main_sel;
    }

    //Determine the main clock source from the CLKSEL bitfield
    sim_id_t main_clk_src;
    switch (m_active_main_sel) {
        case CLKCTRL_CLKSEL_OSCULP32K_gc:
            main_clk_src = ClkSrc_ULP32K; break;
        case CLKCTRL_CLKSEL_XOSC32K_gc:
            main_clk_src = ClkSrc_EXT32K; break;
        case CLKCTRL_CLKSEL_EXTCLK_gc:
            main_clk_src = ClkSrc_EXTCLK; break;
        default: //CLKCTRL_CLKSEL_OSC20M_gc
            main_clk_src = ClkSrc_Main; break;
    }

    //Get the main clock prescaler value
    unsigned long main_pdiv;
    if (TEST_IOREG(MCLKCTRLB, CLKCTRL_PEN)) {
        auto ps_setting = find_reg_config_p(ps_settings, READ_IOREG_F(MCLKCTRLB, CLKCTRL_PDIV));
        main_pdiv = ps_setting ? ps_setting->pdiv : 2;
    } else {
        main_pdiv = 1;
    }

    //Configure the main clock domain
    cm->configure_clock_domain(CycleManager::ReferenceDomain, main_clk_src, main_pdiv);
    logger().dbg("  Main domain : src=%s, div=%lu", main_clk_src.str().c_str(), main_pdiv);

    //Update the status bits for external clocks
    WRITE_IOREG_B(MCLKSTATUS, CLKCTRL_XOSC32KS, ext_cryst_on ? 1 : 0);
    WRITE_IOREG_B(MCLKSTATUS, CLKCTRL_EXTS, m_ext_clk_freq ? 1 : 0);
}
