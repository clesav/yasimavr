/*
 * arch_xt_timer_a.cpp
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

#include "arch_xt_timer_a.h"
#include "arch_xt_timer_b.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"
#include "core/sim_sleep.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(TCA_SINGLE_t, reg))

#define REG_OFS(reg) \
    reg_addr_t(offsetof(TCA_SINGLE_t, reg))

#define TIMER_PRESCALER_MAX         1024
static const uint16_t PrescalerFactors[8] = { 1, 2, 4, 8, 16, 64, 256, 1024 };


ArchXT_TimerA::ArchXT_TimerA(const ArchXT_TimerAConfig& config)
:Peripheral(AVR_IOCTL_TIMER('A', '0'))
,m_config(config)
,m_cnt(0)
,m_per(0)
,m_perbuf(0)
,m_next_event_type(0)
,m_ovf_intflag(false)
{
    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i)
        m_cmp_intflags[i] = new InterruptFlag(false);
}

ArchXT_TimerA::~ArchXT_TimerA()
{
    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i)
        delete m_cmp_intflags[i];
}

bool ArchXT_TimerA::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), TCA_SINGLE_CLKSEL_gm | TCA_SINGLE_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), TCA_SINGLE_WGMODE_gm);
    //CTRLC not implemented
    //CTRLD not implemented
    //CTRLECLR not implemented
    //CTRLESET not implemented
    add_ioreg(REG_ADDR(CTRLFCLR), TCA_SINGLE_PERBV_bm | TCA_SINGLE_CMP0BV_bm |
                                  TCA_SINGLE_CMP1BV_bm | TCA_SINGLE_CMP2BV_bm);
    add_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_PERBV_bm | TCA_SINGLE_CMP0BV_bm |
                                  TCA_SINGLE_CMP1BV_bm | TCA_SINGLE_CMP2BV_bm);
    //EVCTRL not implemented
    add_ioreg(REG_ADDR(INTCTRL), TCA_SINGLE_OVF_bm | TCA_SINGLE_CMP0_bm |
                                 TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm);
    add_ioreg(REG_ADDR(INTFLAGS), TCA_SINGLE_OVF_bm | TCA_SINGLE_CMP0_bm |
                                  TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm);
    //DBGCTRL not implemented
    add_ioreg(REG_ADDR(TEMP));
    add_ioreg(REG_ADDR(CNTL));
    add_ioreg(REG_ADDR(CNTH));
    add_ioreg(REG_ADDR(PERL));
    add_ioreg(REG_ADDR(PERH));
    add_ioreg(REG_ADDR(CMP0L));
    add_ioreg(REG_ADDR(CMP0H));
    add_ioreg(REG_ADDR(CMP1L));
    add_ioreg(REG_ADDR(CMP1H));
    add_ioreg(REG_ADDR(CMP2L));
    add_ioreg(REG_ADDR(CMP2H));
    add_ioreg(REG_ADDR(PERBUFL));
    add_ioreg(REG_ADDR(PERBUFH));
    add_ioreg(REG_ADDR(CMP0BUFL));
    add_ioreg(REG_ADDR(CMP0BUFH));
    add_ioreg(REG_ADDR(CMP1BUFL));
    add_ioreg(REG_ADDR(CMP1BUFH));
    add_ioreg(REG_ADDR(CMP2BUFL));
    add_ioreg(REG_ADDR(CMP2BUFH));

    status &= m_ovf_intflag.init(device,
                                 DEF_REGBIT_B(INTCTRL, TCA_SINGLE_OVF),
                                 DEF_REGBIT_B(INTFLAGS, TCA_SINGLE_OVF),
                                 m_config.iv_ovf);

    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i)
        status &= m_cmp_intflags[i]->init(device,
                                          regbit_t(REG_ADDR(INTCTRL), TCA_SINGLE_CMP0_bp + i),
                                          regbit_t(REG_ADDR(INTFLAGS), TCA_SINGLE_CMP0_bp + i),
                                          m_config.ivs_cmp[i]);

    m_timer.init(*device.cycle_manager(), logger());
    m_timer.signal().connect(*this);

    return status;
}

void ArchXT_TimerA::reset()
{
    Peripheral::reset();
    m_cnt = 0;
    m_per = 0xFFFF;
    m_perbuf = 0xFFFF;
    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i) {
        m_cmp[i] = 0;
        m_cmpbuf[i] = 0;
    }
    m_next_event_type = 0;
    m_timer.reset();
}

bool ArchXT_TimerA::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_TCA_REGISTER_TCB) {
        PrescaledTimer* t = reinterpret_cast<PrescaledTimer*>(data->data.as_ptr());
        if (data->index)
            m_timer.register_chained_timer(*t);
        else
            m_timer.unregister_chained_timer(*t);
        return true;
    }
    return false;
}

uint8_t ArchXT_TimerA::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //16-bits reading of CNT
    if (reg_ofs == REG_OFS(CNTL)) {
        m_timer.update();
        value = m_cnt & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), m_cnt >> 8);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    //16-bits reading of PER
    else if (reg_ofs == REG_OFS(PERL)) {
        value = m_per & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), m_per >> 8);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    //16-bits reading of CMP0,1,2
    else if (reg_ofs >= REG_OFS(CMP0L) && reg_ofs <= REG_OFS(CMP2H)) {
        int index = (reg_ofs - REG_OFS(CMP0L)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0L)) & 1;
        if (high_byte) {
            value = read_ioreg(REG_ADDR(TEMP));
        } else {
            value = m_cmp[index] & 0x00FF;
            write_ioreg(REG_ADDR(TEMP), m_cmp[index] >> 8);
        }
    }

    //16-bits reading of PERBUF
    else if (reg_ofs == REG_OFS(PERBUFL)) {
        value = m_perbuf & 0x00FF;
        write_ioreg(REG_ADDR(TEMP), m_perbuf >> 8);
    }
    else if (reg_ofs == REG_OFS(PERBUFH)) {
        value = read_ioreg(REG_ADDR(TEMP));
    }

    //16-bits reading of CMP0,1,2BUF
    else if (reg_ofs >= REG_OFS(CMP0BUFL) && reg_ofs <= REG_OFS(CMP2BUFH)) {
        int index = (reg_ofs - REG_OFS(CMP0BUFL)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0BUFL)) & 1;
        if (high_byte) {
            value = read_ioreg(REG_ADDR(TEMP));
        } else {
            value = m_cmpbuf[index] & 0x00FF;
            write_ioreg(REG_ADDR(TEMP), m_cmpbuf[index] >> 8);
        }
    }

    return value;
}

void ArchXT_TimerA::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    bool do_reschedule = false;
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        if (data.value & TCA_SINGLE_ENABLE_bm) {
            bitmask_t bm_clksel = DEF_BITMASK_F(TCA_SINGLE_CLKSEL);
            uint16_t factor = PrescalerFactors[bm_clksel.extract(data.value)];
            m_timer.set_prescaler(TIMER_PRESCALER_MAX, factor);
        } else {
            m_timer.set_prescaler(TIMER_PRESCALER_MAX, 0);
        }
    }

    else if (reg_ofs == REG_OFS(CTRLFCLR)) {
        uint8_t v = data.old & ~data.value;
        write_ioreg(REG_ADDR(CTRLFSET), v);
        write_ioreg(REG_ADDR(CTRLFCLR), v);
    }
    else if (reg_ofs == REG_OFS(CTRLFCLR)) {
        uint8_t v = data.old | data.value;
        write_ioreg(REG_ADDR(CTRLFSET), v);
        write_ioreg(REG_ADDR(CTRLFCLR), v);
    }

    //16-bits writing to CNT
    else if (reg_ofs == REG_OFS(CNTL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        m_cnt = read_ioreg(REG_ADDR(TEMP)) | (data.value << 8);
        do_reschedule = true;
    }

    //16-bits writing to PER
    else if (reg_ofs == REG_OFS(PERL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        m_per = read_ioreg(REG_ADDR(TEMP)) | (data.value << 8);
        do_reschedule = true;
    }

    //16-bits writing to CMP0,1,2
    else if (reg_ofs >= REG_OFS(CMP0L) && reg_ofs <= REG_OFS(CMP2H)) {
        int index = (reg_ofs - REG_OFS(CMP0L)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0L)) & 1;
        if (high_byte) {
            m_cmp[index] = read_ioreg(REG_ADDR(TEMP)) | (data.value << 8);
            do_reschedule = true;
        } else {
            write_ioreg(REG_ADDR(TEMP), data.value);
        }
    }

    //16-bits writing to PERBUF
    //It does not trigger a reschedule but only sets the BV flag in CTRLF
    else if (reg_ofs == REG_OFS(PERBUFL)) {
        write_ioreg(REG_ADDR(TEMP), data.value);
    }
    else if (reg_ofs == REG_OFS(PERBUFH)) {
        m_perbuf = read_ioreg(REG_ADDR(TEMP)) | (data.value << 8);
        set_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_PERBV_bp);
        set_ioreg(REG_ADDR(CTRLFCLR), TCA_SINGLE_PERBV_bp);
    }

    //16-bits writing to CMP0,1,2BUF
    //It does not trigger a reschedule but only sets the BV flag in CTRLF
    else if (reg_ofs >= REG_OFS(CMP0BUFL) && reg_ofs <= REG_OFS(CMP2BUFH)) {
        int index = (reg_ofs - REG_OFS(CMP0BUFL)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0BUFL)) & 1;
        if (high_byte) {
            m_cmpbuf[index] = read_ioreg(REG_ADDR(TEMP)) | (data.value << 8);
            set_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_CMP0BV_bp + index);
            set_ioreg(REG_ADDR(CTRLFCLR), TCA_SINGLE_CMP0BV_bp + index);
        } else {
            write_ioreg(REG_ADDR(TEMP), data.value);
        }
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_ovf_intflag.update_from_ioreg();
        for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i)
            m_cmp_intflags[i]->update_from_ioreg();
    }

    //If we're writing a 1 to a interrupt flag bit, it clears the bit and cancels the interrupt
    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        write_ioreg(addr, 0);
        m_ovf_intflag.clear_flag(EXTRACT_B(data.value, TCA_SINGLE_OVF));
        for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i) {
            bitmask_t bm = bitmask_t(TCA_SINGLE_CMP0_bp + i);
            m_cmp_intflags[i]->clear_flag(bm.extract(data.value));
        }
    }

    if (do_reschedule)
        m_timer.set_timer_delay(delay_to_event());
}

/*
 * Defines the types of 'event' that can be triggered by the counter
 */
enum TimerEventType {
    TimerEventComp0     = 0x01,
    TimerEventComp1     = 0x02,
    TimerEventComp2     = 0x04,
    TimerEventPer       = 0x80,
};

/*
 * Calculates the delay in source ticks and the type of the next timer/counter event
 */
uint32_t ArchXT_TimerA::delay_to_event()
{
    int ticks_to_max = PrescaledTimer::ticks_to_event(m_cnt, 0xFFFF, 0x10000);
    int ticks_to_next_event = ticks_to_max;

    int ticks_to_per = PrescaledTimer::ticks_to_event(m_cnt, m_per, 0x10000);
    if (ticks_to_per < ticks_to_next_event)
        ticks_to_next_event = ticks_to_per;

    int ticks_to_comp[3];
    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i) {
        int t = PrescaledTimer::ticks_to_event(m_cnt, m_cmp[i], 0x10000);
        ticks_to_comp[i] = t;
        if (t < ticks_to_next_event)
            ticks_to_next_event = t;
    }

    m_next_event_type = 0;

    if (ticks_to_next_event == ticks_to_per)
        m_next_event_type |= TimerEventPer;

    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i) {
        if (ticks_to_next_event == ticks_to_comp[i])
            m_next_event_type |= (TimerEventComp0 << i);
    }

    return (uint32_t)ticks_to_next_event;
}

void ArchXT_TimerA::raised(const signal_data_t& sigdata, int)
{
    m_cnt += sigdata.data.as_uint();

    if (!sigdata.index) return;

    logger().dbg("Processing events %02x", m_next_event_type);

    if (m_next_event_type & TimerEventPer) {
        m_cnt = 0;
        m_ovf_intflag.set_flag();
        update_16bits_buffers();
    }

    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i) {
        if (m_next_event_type & (TimerEventComp0 << i))
            m_cmp_intflags[i]->set_flag();
    }

    //Reconfigure the timer delay to the next event
    m_timer.set_timer_delay(delay_to_event());
}

void ArchXT_TimerA::update_16bits_buffers()
{
    m_per = m_perbuf;
    for (int i = 0; i < AVR_TCA_CMP_CHANNEL_COUNT; ++i)
        m_cmp[i] = m_cmpbuf[i];

    clear_ioreg(REG_ADDR(CTRLFSET));
    clear_ioreg(REG_ADDR(CTRLFCLR));
}

void ArchXT_TimerA::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::Idle)
        m_timer.set_paused(on);
}
