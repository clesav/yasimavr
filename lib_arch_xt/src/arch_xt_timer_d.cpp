/*
 * arch_xt_timer_d.cpp
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

#include "arch_xt_timer_d.h"
#include "avr_io/io_tcd.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(TCD_t, reg))

#define REG_OFS(reg) \
    reg_addr_t(offsetof(TCD_t, reg))

#define READ_IOREG_16(reg) \
    (((uint16_t)READ_IOREG(reg ## L)) | (((uint16_t)READ_IOREG(reg ## H)) << 8))


#define COUNTER_PRESCALER_MAX         32
static const unsigned int CounterPrescalerFactors[4] = { 1, 4, 32, 1 /*placeholder*/ };
#define SYNC_PRESCALER_MAX            8
static const unsigned int SyncPrescalerFactors[4] = { 1, 2, 4, 8 };


enum CompareChannel {
    Comp_ASET,
    Comp_ACLR,
    Comp_BSET,
    Comp_BCLR
};


ArchXT_TimerD::ArchXT_TimerD(const ArchXT_TimerDConfig& config)
:Peripheral(AVR_IOCTL_TIMER('D', '0'))
,m_config(config)
,m_ovf_intflag(false)
,m_counter(0x1000, 4)
,m_wgmode(0)
,m_cycle_state(0)
,m_counter_hook(*this, &ArchXT_TimerD::counter_raised)
{}


bool ArchXT_TimerD::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), TCD_CLKSEL_gm | TCD_CNTPRES_gm | TCD_SYNCPRES_gm | TCD_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), TCD_WGMODE_gm);
    add_ioreg(REG_ADDR(CTRLC), TCD_CMPDSEL_bm | TCD_CMPCSEL_bm | TCD_FIFTY_bm | TCD_AUPDATE_bm | TCD_CMPOVR_bm);
    add_ioreg(REG_ADDR(CTRLD), TCD_CMPBVAL_gm | TCD_CMPAVAL_gm);
    add_ioreg(REG_ADDR(CTRLE), TCD_DISEOC_bm | TCD_SCAPTUREB_bm | TCD_SCAPTUREA_bm | TCD_RESTART_bm | TCD_SYNC_bm | TCD_SYNCEOC_bm);
    add_ioreg(REG_ADDR(EVCTRLA), TCD_CFG_gm | TCD_EDGE_bm | TCD_ACTION_bm | TCD_TRIGEI_bm);
    add_ioreg(REG_ADDR(EVCTRLB), TCD_CFG_gm | TCD_EDGE_bm | TCD_ACTION_bm | TCD_TRIGEI_bm);
    add_ioreg(REG_ADDR(INTCTRL), TCD_TRIGB_bm | TCD_TRIGA_bm | TCD_OVF_bm);
    add_ioreg(REG_ADDR(INTFLAGS), TCD_TRIGB_bm | TCD_TRIGA_bm | TCD_OVF_bm, IORegister::Strobe);
    add_ioreg(REG_ADDR(STATUS), TCD_PWMACTB_bm | TCD_PWMACTA_bm);
    add_ioreg(REG_ADDR(STATUS), TCD_CMDRDY_bm | TCD_ENRDY_bm, IORegister::RO);
    add_ioreg(REG_ADDR(INPUTCTRLA), TCD_INPUTMODE_gm);
    add_ioreg(REG_ADDR(INPUTCTRLB), TCD_INPUTMODE_gm);
    add_ioreg(REG_ADDR(FAULTCTRL), TCD_CMPDEN_bm | TCD_CMPCEN_bm | TCD_CMPBEN_bm | TCD_CMPAEN_bm |
                                   TCD_CMPD_bm | TCD_CMPC_bm | TCD_CMPB_bm | TCD_CMPA_bm);
    add_ioreg(REG_ADDR(DLYCTRL), TCD_DLYPRESC_gm | TCD_DLYTRIG_gm | TCD_DLYSEL_gm);
    add_ioreg(REG_ADDR(DLYVAL), TCD_DLYVAL_gm);
    add_ioreg(REG_ADDR(DITCTRL), TCD_DITHERSEL_gm);
    add_ioreg(REG_ADDR(DITVAL), TCD_DITHER_gm);
    //DBGCTRL not supported
    add_ioreg(REG_ADDR(CAPTUREAL), 0xFF, IORegister::RO);
    add_ioreg(REG_ADDR(CAPTUREAH), 0x0F, IORegister::RO);
    add_ioreg(REG_ADDR(CAPTUREBL), 0xFF, IORegister::RO);
    add_ioreg(REG_ADDR(CAPTUREBH), 0x0F, IORegister::RO);
    add_ioreg(REG_ADDR(CMPASETL), 0xFF);
    add_ioreg(REG_ADDR(CMPASETH), 0x0F);
    add_ioreg(REG_ADDR(CMPBSETL), 0xFF);
    add_ioreg(REG_ADDR(CMPBSETH), 0x0F);
    add_ioreg(REG_ADDR(CMPACLRL), 0xFF);
    add_ioreg(REG_ADDR(CMPACLRH), 0x0F);
    add_ioreg(REG_ADDR(CMPBCLRL), 0xFF);
    add_ioreg(REG_ADDR(CMPBCLRH), 0x0F);

    //Initialise the interrupt flags
    status &= m_ovf_intflag.init(device,
                                 REG_ADDR(INTCTRL), REG_ADDR(INTFLAGS),
                                 TCD_OVF_bm,
                                 m_config.iv_ovf);

    //Initialise the timer/counter
    m_counter.init(*device.cycle_manager(), logger());
    m_counter.signal().connect(m_counter_hook);

    return status;
}


void ArchXT_TimerD::reset(int)
{
    m_counter.reset();
    m_wgmode = 0;
    m_cycle_state = 0;

    SET_IOREG(STATUS, TCD_CMDRDY);
    SET_IOREG(STATUS, TCD_ENRDY);
}


void ArchXT_TimerD::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        if (data.posedge() & TCD_ENABLE_bm)
            start_counter();
        else if (data.negedge() & TCD_ENABLE_bm)
            stop_counter();
    }
}


void ArchXT_TimerD::start_counter()
{
    m_counter.set_tick_source(TimerCounter::Tick_Timer);
    m_counter.prescaler().set_prescaler(COUNTER_PRESCALER_MAX * SYNC_PRESCALER_MAX,
                                        CounterPrescalerFactors[READ_IOREG_F(CTRLA, TCD_CNTPRES)] *
                                        SyncPrescalerFactors[READ_IOREG_F(CTRLA, TCD_SYNCPRES)]);

    m_wgmode = READ_IOREG(CTRLB) & TCD_WGMODE_gm;
    if (m_wgmode == TCD_WGMODE_DS_gc) {

        m_counter.set_slope_mode(TimerCounter::Slope_Double);
        m_counter.set_countdown(true);
        m_counter.set_counter(READ_IOREG_16(CMPBCLR));

    } else {

        m_counter.set_slope_mode(TimerCounter::Slope_Up);
        m_counter.set_counter(0);

    }

    m_counter.set_comp_value(Comp_ASET, READ_IOREG_16(CMPASET));
    m_counter.set_comp_value(Comp_ACLR, READ_IOREG_16(CMPACLR));
    m_counter.set_comp_value(Comp_BSET, READ_IOREG_16(CMPBSET));
    m_counter.set_comp_value(Comp_BCLR, READ_IOREG_16(CMPBCLR));

    m_cycle_state = 0;
    configure_counter_cycle_state();

    m_counter.reschedule();
}


void ArchXT_TimerD::stop_counter()
{
    m_counter.update();
    m_counter.set_tick_source(TimerCounter::Tick_Stopped);
    m_counter.reschedule();
}


void ArchXT_TimerD::configure_counter_cycle_state()
{
    switch(m_wgmode) {
        case TCD_WGMODE_ONERAMP_gc: {
            m_counter.set_top(m_counter.comp_value(Comp_BCLR));
            for (size_t i = 0; i < 4; ++i)
                m_counter.set_comp_enabled(i, true);
        } break;

        case TCD_WGMODE_TWORAMP_gc: {
            m_counter.set_top(m_cycle_state ? m_counter.comp_value(Comp_BCLR) : m_counter.comp_value(Comp_ACLR));
            m_counter.set_comp_enabled(0, !m_cycle_state);
            m_counter.set_comp_enabled(1, !m_cycle_state);
            m_counter.set_comp_enabled(2, m_cycle_state);
            m_counter.set_comp_enabled(3, m_cycle_state);
        } break;

        case TCD_WGMODE_FOURRAMP_gc: {
            m_counter.set_top(m_counter.comp_value(m_cycle_state));
            for (size_t i = 0; i < 4; ++i)
                m_counter.set_comp_enabled(i, i == m_cycle_state);
        } break;

        case TCD_WGMODE_DS_gc: {
            m_counter.set_top(m_counter.comp_value(Comp_BCLR));
            m_counter.set_comp_enabled(0, true);
            m_counter.set_comp_enabled(1, false);
            m_counter.set_comp_enabled(2, true);
            m_counter.set_comp_enabled(3, false);
        } break;
    };
}


void ArchXT_TimerD::counter_raised(const signal_data_t& sigdata, int)
{
    if (sigdata.sigid == TimerCounter::Signal_CompMatch) {
        if (sigdata.index == m_cycle_state) {
            m_cycle_state = (m_cycle_state + 1) % 4;

            if (!m_cycle_state)
                m_ovf_intflag.set_flag();

            configure_counter_cycle_state();
        }
    }
}
