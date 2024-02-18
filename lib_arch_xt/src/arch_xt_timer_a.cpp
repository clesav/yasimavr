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


enum OutputChange {
    Output_NoChange = 0,
    Output_Clear,
    Output_Set,
    Output_Toggle
};


typedef ArchXT_TimerAConfig CFG;


/*
 * Utility function that translates a channel number into the position of
 * the corresponding CMPxOV bit.
 */
static uint8_t index_to_CMPxOV_bit(bool split_mode, int index)
{
    if (!split_mode)
        return TCA_SINGLE_CMP0OV_bp + index;
    else if (index < CFG::CompareChannelCount)
        return TCA_SPLIT_LCMP0OV_bp + index;
    else
        return TCA_SPLIT_HCMP0OV_bp + index - CFG::CompareChannelCount;
}


/*
 * Utility function that translates a channel number into the position of
 * the corresponding CMPxEN bit.
 */
static uint8_t index_to_CMPxEN_bit(bool split_mode, int index)
{
    if (!split_mode)
        return TCA_SINGLE_CMP0EN_bp + index;
    else if (index < CFG::CompareChannelCount)
        return TCA_SPLIT_LCMP0EN_bp + index;
    else
        return TCA_SPLIT_HCMP0EN_bp + index - CFG::CompareChannelCount;
}


class ArchXT_TimerA::EventHook : public SignalHook {

public:

    explicit EventHook(ArchXT_TimerA& tmr) : m_tmr(tmr) {}

    virtual void raised(const signal_data_t& sigdata, int hooktag) override
    {
        m_tmr.event_raised(sigdata, hooktag);
    }

private:

    ArchXT_TimerA& m_tmr;

};


ArchXT_TimerA::ArchXT_TimerA(const ArchXT_TimerAConfig& config)
:Peripheral(AVR_IOCTL_TIMER('A', '0'))
,m_config(config)
,m_ovf_intflag(false)
,m_hunf_intflag(false)
,m_cmp_intflags{
    InterruptFlag(false),
    InterruptFlag(false),
    InterruptFlag(false),
}
,m_split_mode(false)
,m_sgl_counter(0x10000, 3)
,m_lo_counter(0x100, 3)
,m_hi_counter(0x100, 3)
,m_wgmode(TCA_SINGLE_WGMODE_NORMAL_gc)
,m_EIA_state(false)
,m_EIB_state(false)
,m_timer_block(false)
{
    for (int i = 0; i < CFG::CompareChannelCount; ++i)
        m_cmp_intflags[i].set_clear_on_ack(false);

    m_event_hook = new EventHook(*this);
}


ArchXT_TimerA::~ArchXT_TimerA()
{
    delete m_event_hook;
}


bool ArchXT_TimerA::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Bitmask for CTRLA. Add RUNSTDBY for V2
    uint8_t ctrla_fields = TCA_SINGLE_CLKSEL_gm | TCA_SINGLE_ENABLE_bm;
    if (m_config.version >= CFG::V2) ctrla_fields |= TCA_SINGLE_RUNSTDBY_bm;
    add_ioreg(REG_ADDR(CTRLA), ctrla_fields);

    add_ioreg(REG_ADDR(CTRLB), TCA_SINGLE_WGMODE_gm | TCA_SINGLE_ALUPD_bm |
                               TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP2EN_bm |
                               TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_LCMP1EN_bm | TCA_SPLIT_LCMP2EN_bm |
                               TCA_SPLIT_HCMP0EN_bm | TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm);
    add_ioreg(REG_ADDR(CTRLC), TCA_SINGLE_CMP0OV_bm | TCA_SINGLE_CMP1OV_bm | TCA_SINGLE_CMP2OV_bm |
                               TCA_SPLIT_LCMP0OV_bm | TCA_SPLIT_LCMP1OV_bm | TCA_SPLIT_LCMP2OV_bm |
                               TCA_SPLIT_HCMP0OV_bm | TCA_SPLIT_HCMP1OV_bm | TCA_SPLIT_HCMP2OV_bm);
    add_ioreg(REG_ADDR(CTRLD), TCA_SPLIT_SPLITM_bm);
    add_ioreg(REG_ADDR(CTRLECLR), TCA_SINGLE_DIR_bm | TCA_SINGLE_LUPD_bm | TCA_SINGLE_CMD_gm |
                                  TCA_SPLIT_CMDEN_gm);
    add_ioreg(REG_ADDR(CTRLESET), TCA_SINGLE_DIR_bm | TCA_SINGLE_LUPD_bm | TCA_SINGLE_CMD_gm |
                                  TCA_SPLIT_CMDEN_gm);
    add_ioreg(REG_ADDR(CTRLFCLR), TCA_SINGLE_PERBV_bm |
                                  TCA_SINGLE_CMP0BV_bm | TCA_SINGLE_CMP1BV_bm | TCA_SINGLE_CMP2BV_bm);
    add_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_PERBV_bm |
                                  TCA_SINGLE_CMP0BV_bm | TCA_SINGLE_CMP1BV_bm | TCA_SINGLE_CMP2BV_bm);

    //Bitmask for EVCTRL. Add CNTBEI and EVACTB for V2
    uint8_t evctrl_fields = TCA_SINGLE_CNTAEI_bm | TCA_SINGLE_EVACTA_gm;
    if (m_config.version >= CFG::V2) evctrl_fields |= TCA_SINGLE_CNTBEI_bm | TCA_SINGLE_EVACTB_gm;
    add_ioreg(REG_ADDR(EVCTRL), evctrl_fields);

    add_ioreg(REG_ADDR(INTCTRL), TCA_SINGLE_OVF_bm |
                                 TCA_SINGLE_CMP0_bm | TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm |
                                 TCA_SPLIT_HUNF_bm);
    add_ioreg(REG_ADDR(INTFLAGS), TCA_SINGLE_OVF_bm |
                                  TCA_SINGLE_CMP0_bm | TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm |
                                  TCA_SPLIT_HUNF_bm);
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

    //Initialise the interrupt flags OVF (a.k.a LUNF) and HUNF
    status &= m_ovf_intflag.init(device,
                                 DEF_REGBIT_B(INTCTRL, TCA_SINGLE_OVF),
                                 DEF_REGBIT_B(INTFLAGS, TCA_SINGLE_OVF),
                                 m_config.iv_ovf);
    status &= m_hunf_intflag.init(device,
                                  DEF_REGBIT_B(INTCTRL, TCA_SPLIT_HUNF),
                                  DEF_REGBIT_B(INTFLAGS, TCA_SPLIT_HUNF),
                                  m_config.iv_hunf);

    //Initialise the interrupt flags for the compare channels
    for (int i = 0; i < CFG::CompareChannelCount; ++i)
        status &= m_cmp_intflags[i].init(device,
                                         regbit_t(REG_ADDR(INTCTRL), TCA_SINGLE_CMP0_bp + i),
                                         regbit_t(REG_ADDR(INTFLAGS), TCA_SINGLE_CMP0_bp + i),
                                         m_config.ivs_cmp[i]);

    //Initialise and chain the timer/counters
    m_timer.init(*device.cycle_manager(), logger());

    m_sgl_counter.init(*device.cycle_manager(), logger());
    m_timer.register_chained_timer(m_sgl_counter.prescaler());
    m_sgl_counter.signal().connect(*this, Tag_Single);

    m_lo_counter.init(*device.cycle_manager(), logger());
    m_timer.register_chained_timer(m_lo_counter.prescaler());
    m_lo_counter.signal().connect(*this, Tag_SplitLow);

    m_hi_counter.init(*device.cycle_manager(), logger());
    m_timer.register_chained_timer(m_hi_counter.prescaler());
    m_hi_counter.signal().connect(*this, Tag_SplitHigh);

    return status;
}


void ArchXT_TimerA::reset()
{
    m_timer.reset();

    m_split_mode = false;

    //Reset the period register value
    m_per = { 0xFFFF, 0xFFFF, false };
    WRITE_IOREG(PERL, 0xFF);
    WRITE_IOREG(PERH, 0xFF);

    //Reset all Output Compare buffered registers, interrupts and signal values
    for (int i = 0; i < CFG::CompareChannelCount; ++i) {
        m_cmp[i] = { 0x0000, 0x0000, false };
        m_cmp_intflags[i].update_from_ioreg();
        m_signal.raise(Signal_CompareOutput, vardata_t(), i);
        m_signal.raise(Signal_CompareOutput, vardata_t(), i + CFG::CompareChannelCount);
    }

    //Reset the single mode counter to default settings
    m_sgl_counter.reset();
    m_sgl_counter.prescaler().set_prescaler(1, 1);
    m_sgl_counter.set_tick_source(TimerCounter::Tick_Timer);

    //Reset the split counters to default settings
    m_lo_counter.reset();
    m_lo_counter.prescaler().set_prescaler(1, 1);
    m_lo_counter.set_countdown(true);
    m_hi_counter.reset();
    m_hi_counter.prescaler().set_prescaler(1, 1);
    m_hi_counter.set_countdown(true);

    //Reset the compare modules of the counters
    for (int i = 0; i < CFG::CompareChannelCount; ++i) {
        m_sgl_counter.set_comp_enabled(i, true);
        m_lo_counter.set_comp_enabled(i, true);
        m_hi_counter.set_comp_enabled(i, true);
    }

    //Apply the counter default configuration
    m_sgl_counter.reschedule();
    m_lo_counter.reschedule();
    m_hi_counter.reschedule();

    //Reset the Waveform mode
    m_wgmode = TCA_SINGLE_WGMODE_NORMAL_gc;

    //Reset the event state machines
    m_EIA_state = false;
    m_EIB_state = false;
    m_timer_block = false;
}


bool ArchXT_TimerA::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    else if (req == AVR_CTLREQ_TCA_REGISTER_TCB) {
        PrescaledTimer* t = reinterpret_cast<PrescaledTimer*>(data->data.as_ptr());
        if (data->index)
            m_timer.register_chained_timer(*t);
        else
            m_timer.unregister_chained_timer(*t);
        return true;
    }
    else if (req == AVR_CTLREQ_TCA_GET_EVENT_HOOK) {
        data->data = m_event_hook;
        return true;
    }
    return false;
}

/*
 * Handler for reading registers.
 * Delegates to one of the sub-handler depending on the peripheral mode
*/
uint8_t ArchXT_TimerA::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;
    if (m_split_mode)
        return read_ioreg_split(reg_ofs, value);
    else
        return read_ioreg_single(reg_ofs, value);
}

/*
 * Sub-handler for reading registers in single mode
*/
uint8_t ArchXT_TimerA::read_ioreg_single(reg_addr_t reg_ofs, uint8_t value)
{
    //16-bits reading of CNT/LCNT/HCNT
    if (reg_ofs == REG_OFS(CNTL)) {
        m_sgl_counter.update();
        uint16_t v = m_sgl_counter.counter();
        value = v & 0x00FF;
        WRITE_IOREG(TEMP, v >> 8);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        value = READ_IOREG(TEMP);
    }

    //16-bits reading of PER/LPER/HPER
    else if (reg_ofs == REG_OFS(PERL)) {
        value = m_per.value & 0x00FF;
        WRITE_IOREG(TEMP, m_per.value >> 8);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        value = READ_IOREG(TEMP);
    }

    //16-bits reading of CMP0,1,2
    else if (REG_OFS(CMP0L) <= reg_ofs && reg_ofs <= REG_OFS(CMP2H)) {
        int index = (reg_ofs - REG_OFS(CMP0L)) >> 1;
        bool high_byte = bool((reg_ofs - REG_OFS(CMP0L)) & 1);
        if (high_byte) {
            value = READ_IOREG(TEMP);
        } else {
            value = m_cmp[index].value & 0x00FF;
            WRITE_IOREG(TEMP, m_cmp[index].value >> 8);
        }
    }

    //16-bits reading of PERBUF
    else if (reg_ofs == REG_OFS(PERBUFL)) {
        value = m_per.buffer & 0x00FF;
        WRITE_IOREG(TEMP, m_per.buffer >> 8);
    }
    else if (reg_ofs == REG_OFS(PERBUFH)) {
        value = READ_IOREG(TEMP);
    }

    //16-bits reading of CMP0,1,2BUF
    else if (REG_OFS(CMP0BUFL) <= reg_ofs && reg_ofs <= REG_OFS(CMP2BUFH)) {
        int index = (reg_ofs - REG_OFS(CMP0BUFL)) >> 1;
        bool high_byte = bool((reg_ofs - REG_OFS(CMP0BUFL)) & 1);
        if (high_byte) {
            value = READ_IOREG(TEMP);
        } else {
            value = m_cmp[index].buffer & 0x00FF;
            WRITE_IOREG(TEMP, m_cmp[index].buffer >> 8);
        }
    }

    return value;
}


uint8_t ArchXT_TimerA::read_ioreg_split(reg_addr_t reg_ofs, uint8_t value)
{
    if (reg_ofs == REG_OFS(CNTL)) {
        m_lo_counter.update();
        value = m_lo_counter.counter();
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        m_hi_counter.update();
        value = m_hi_counter.counter();
    }

    return value;
}


void ArchXT_TimerA::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        if (data.value & TCA_SINGLE_ENABLE_bm) {
            int factor = PrescalerFactors[EXTRACT_F(data.value, TCA_SINGLE_CLKSEL)];
            m_timer.set_prescaler(TIMER_PRESCALER_MAX, factor);
        } else {
            m_timer.set_prescaler(TIMER_PRESCALER_MAX, 0);
        }
    }

    else if (reg_ofs == REG_OFS(CTRLD)) {
        set_peripheral_mode(data.value & TCA_SINGLE_SPLITM_bm);
    }

    //CTRLE Register, defined by SET and CLR registers
    else if (reg_ofs == REG_OFS(CTRLESET)) {
        //Update the register value by setting bits but the CMD field always ends up zero
        uint8_t v = data.old | data.value;
        v &= ~TCA_SINGLE_CMD_gm;
        WRITE_IOREG(CTRLESET, v);
        WRITE_IOREG(CTRLECLR, v);

        //Extract the command code to execute
        uint8_t cmd = data.value & TCA_SINGLE_CMD_gm;
        bool cmden;
        if (m_split_mode)
            cmden = data.value & TCA_SPLIT_CMDEN_gm;
        else
            cmden = true;

        //Execute the command, returning a boolean if we can update the direction (single mode only)
        if (execute_command(cmd, cmden) && !m_split_mode)
            //Update the direction
            set_direction(v & TCA_SINGLE_DIR_bm, true);
    }

    else if (reg_ofs == REG_OFS(CTRLECLR)) {
        //Update the register value by clearing bits
        uint8_t v = data.old & ~data.value;
        WRITE_IOREG(CTRLESET, v);
        WRITE_IOREG(CTRLECLR, v);

        //Update the direction (single mode only)
        if (!m_split_mode)
            set_direction(v & TCA_SINGLE_DIR_bm, true);
    }

    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_ovf_intflag.update_from_ioreg();
        m_hunf_intflag.update_from_ioreg();
        for (auto& cmp : m_cmp_intflags)
            cmp.update_from_ioreg();
    }

    //If we're writing a 1 to the interrupt flag bit, it clears the bit and cancels the interrupt
    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        if (data.value & TCA_SINGLE_OVF_bm)
            m_ovf_intflag.clear_flag();

        if (data.value & TCA_SPLIT_HUNF_bm)
            m_hunf_intflag.clear_flag();

        for (int i = 0; i < CFG::CompareChannelCount; ++i) {
            if (data.value & (TCA_SINGLE_CMP0_bm << i))
                m_cmp_intflags[i].clear_flag();
        }
    }

    //All other registers are treated differently according to the single/split mode
    else if (m_split_mode)
        write_ioreg_split(reg_ofs, data);
    else
        write_ioreg_single(reg_ofs, data);
}


void ArchXT_TimerA::write_ioreg_single(reg_addr_t reg_ofs, const ioreg_write_t& data)
{
    if (reg_ofs == REG_OFS(CTRLB)) {
        m_wgmode = data.value & TCA_SINGLE_WGMODE_gm;
        update_ALUPD_status();
        configure_single_counter();
        //Ensure the compare outputs are updated with the CMPxEN new values
        update_compare_outputs();
    }

    else if (reg_ofs == REG_OFS(CTRLC)) {
        if (TEST_IOREG(CTRLA, TCA_SINGLE_ENABLE)) {
            //If the timer is enabled, the CMPxOV bits are read-only so we restore the old values
            for (int i = 0; i < CFG::CompareChannelCount; ++i) {
                bitmask_t ov_bm = bitmask_t(index_to_CMPxOV_bit(false, i));
                write_ioreg(REG_ADDR(CTRLC), ov_bm, ov_bm.extract(data.old));
            }
        } else {
            //If the timer is disabled, the CMPxOV bits control the compare outputs.
            //Ensure the compare outputs are updated with the CMPxOV new values
            update_compare_outputs();
        }
    }

    else if (reg_ofs == REG_OFS(CTRLFSET)) {
        uint8_t v = data.old | data.value;
        WRITE_IOREG(CTRLFSET, v);
        WRITE_IOREG(CTRLFCLR, v);
    }
    else if (reg_ofs == REG_OFS(CTRLFCLR)) {
        uint8_t v = data.old & ~data.value;
        WRITE_IOREG(CTRLFSET, v);
        WRITE_IOREG(CTRLFCLR, v);
    }

    else if (reg_ofs == REG_OFS(EVCTRL)) {
        m_sgl_counter.update();
        update_timer_block(data.value);
        update_tick_sources();
        m_sgl_counter.reschedule();
        update_compare_outputs();
    }

    //16-bits writing to CNT
    else if (reg_ofs == REG_OFS(CNTL)) {
        WRITE_IOREG(TEMP, data.value);
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        //Collate the 16-bits value of CNT using the TEMP register
        m_sgl_counter.update();
        m_sgl_counter.set_counter(READ_IOREG(TEMP) | (data.value << 8));
        m_sgl_counter.reschedule();
    }

    //16-bits writing to PER
    else if (reg_ofs == REG_OFS(PERL)) {
        WRITE_IOREG(TEMP, data.value);
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        m_per.value = READ_IOREG(TEMP) | (data.value << 8);
        configure_single_counter();
    }

    //16-bits writing to CMPx
    else if (REG_OFS(CMP0L) <= reg_ofs && reg_ofs <= REG_OFS(CMP2H)) {
        int index = (reg_ofs - REG_OFS(CMP0L)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0L)) & 1;
        if (high_byte) {
            m_cmp[index].value = READ_IOREG(TEMP) | (data.value << 8);
            configure_single_counter();
        } else {
            WRITE_IOREG(TEMP, data.value);
        }
    }

    //16-bits writing to PERBUF
    else if (reg_ofs == REG_OFS(PERBUFL)) {
        WRITE_IOREG(TEMP, data.value);
    }
    else if (reg_ofs == REG_OFS(PERBUFH)) {
        m_per.buffer = READ_IOREG(TEMP) | (data.value << 8);
        m_per.flag = true;
        SET_IOREG(CTRLFSET, TCA_SINGLE_PERBV);
        SET_IOREG(CTRLFCLR, TCA_SINGLE_PERBV);
        update_ALUPD_status();
    }

    //16-bits writing to CMPxBUF
    else if (REG_OFS(CMP0BUFL) <= reg_ofs && reg_ofs <= REG_OFS(CMP2BUFH)) {
        int index = (reg_ofs - REG_OFS(CMP0BUFL)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0BUFL)) & 1;
        if (high_byte) {
            m_cmp[index].buffer = READ_IOREG(TEMP) | (data.value << 8);
            m_cmp[index].flag = true;
            set_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_CMP0BV_bp + index);
            set_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_CMP0BV_bp + index);
            update_ALUPD_status();
        } else {
            WRITE_IOREG(TEMP, data.value);
        }
    }
}


void ArchXT_TimerA::write_ioreg_split(reg_addr_t reg_ofs, const ioreg_write_t& data)
{
    if (reg_ofs == REG_OFS(CTRLB)) {
        update_compare_outputs();
    }

    else if (reg_ofs == REG_OFS(CTRLC)) {
        if (TEST_IOREG(CTRLA, TCA_SPLIT_ENABLE)) {
            //If the timer is enabled, the CMPxOV bits are read-only so we restore the old values
            for (int i = 0; i < CFG::CompareChannelCount * 2; ++i) {
                bitmask_t ov_bm = bitmask_t(index_to_CMPxOV_bit(true, i));
                write_ioreg(REG_ADDR(CTRLC), ov_bm, ov_bm.extract(data.old));
            }
        } else {
            //If the timer is disabled, the CMPxOV bits control the compare outputs.
            //Ensure the compare outputs are updated with the CMPxOV new values
            update_compare_outputs();
        }
    }

    else if (reg_ofs == REG_OFS(CNTL)) {
        m_lo_counter.update();
        m_lo_counter.set_counter(data.value);
        m_lo_counter.reschedule();
    }
    else if (reg_ofs == REG_OFS(CNTH)) {
        m_hi_counter.update();
        m_hi_counter.set_counter(data.value);
        m_hi_counter.reschedule();
    }

    //16-bits writing to PER
    else if (reg_ofs == REG_OFS(PERL)) {
        m_per.value = (m_per.value & 0xFF00) | data.value;
        m_lo_counter.update();
        m_lo_counter.set_top(data.value);
        m_lo_counter.reschedule();
    }
    else if (reg_ofs == REG_OFS(PERH)) {
        m_per.value = (m_per.value & 0x00FF) | (data.value << 8);
        m_hi_counter.update();
        m_hi_counter.set_top(data.value);
        m_hi_counter.reschedule();
    }

    //16-bits writing to CMPx
    else if (REG_OFS(CMP0L) <= reg_ofs && reg_ofs <= REG_OFS(CMP2H)) {
        int index = (reg_ofs - REG_OFS(CMP0L)) >> 1;
        bool high_byte = (reg_ofs - REG_OFS(CMP0L)) & 1;
        auto& cmp = m_cmp[index];
        if (high_byte) {
            cmp.value = (cmp.value & 0xFF00) | data.value;
            m_hi_counter.update();
            m_hi_counter.set_comp_value(index, data.value);
            m_hi_counter.reschedule();
        } else {
            cmp.value = (cmp.value & 0x00FF) | (data.value << 8);
            m_lo_counter.update();
            m_lo_counter.set_comp_value(index, data.value);
            m_lo_counter.reschedule();
        }
    }
}


void ArchXT_TimerA::update_ALUPD_status()
{
    if (TEST_IOREG(CTRLB, TCA_SINGLE_ALUPD)) {
        //The LUPD bit is set if at least one register buffer flag is not set
        bool lupd = !m_per.flag;
        for (auto& cmp : m_cmp)
            lupd |= !cmp.flag;

        WRITE_IOREG_B(CTRLESET, TCA_SINGLE_LUPD, lupd);
        WRITE_IOREG_B(CTRLECLR, TCA_SINGLE_LUPD, lupd);
    }
}


void ArchXT_TimerA::update_buffered_registers()
{
    if (TEST_IOREG(CTRLESET, TCA_SINGLE_LUPD)) return;

    bool reconfig = false;

    if (m_per.flag) {
        m_per.value = m_per.buffer;
        m_per.flag = false;
        CLEAR_IOREG(CTRLFSET, TCA_SINGLE_PERBV);
        CLEAR_IOREG(CTRLFCLR, TCA_SINGLE_PERBV);
        reconfig = true;
    }

    for (int i = 0; i < CFG::CompareChannelCount; ++i) {
        if (m_cmp[i].flag) {
            m_cmp[i].value = m_cmp[i].buffer;
            m_cmp[i].flag = false;
            clear_ioreg(REG_ADDR(CTRLFSET), TCA_SINGLE_CMP0BV_bp + i);
            clear_ioreg(REG_ADDR(CTRLFCLR), TCA_SINGLE_CMP0BV_bp + i);
            if (!i)
                reconfig = true;
        }
    }

    if (reconfig)
        configure_single_counter();

    update_ALUPD_status();
}


void ArchXT_TimerA::set_peripheral_mode(bool split_mode)
{
    m_sgl_counter.update();
    m_lo_counter.update();
    m_hi_counter.update();

    if (split_mode && !m_split_mode) {
        uint16_t c = m_sgl_counter.counter();
        m_lo_counter.set_counter(c & 0x00FF);
        m_hi_counter.set_counter(c >> 8);

        m_lo_counter.set_top(m_per.value & 0x00FF);
        m_hi_counter.set_top(m_per.value >> 8);

        for (int i = 0; i < CFG::CompareChannelCount; ++i) {
            m_lo_counter.set_comp_value(i, m_cmp[i].value & 0x00FF);
            m_hi_counter.set_comp_value(i, m_cmp[i].value >> 8);
        }
    }

    else if (m_split_mode && !split_mode) {
        long cntl = m_lo_counter.counter();
        long cnth = m_hi_counter.counter();
        m_sgl_counter.set_counter((cnth << 8) | cntl);

        configure_single_counter();
    }

    m_split_mode = split_mode;

    update_tick_sources();

    m_sgl_counter.reschedule();
    m_lo_counter.reschedule();
    m_hi_counter.reschedule();

    //Ensure the compare outputs are up to date with the controlling bits and the peripheral mode
    update_compare_outputs();
}


void ArchXT_TimerA::update_tick_sources()
{
    if (m_split_mode) {
        m_sgl_counter.set_tick_source(TimerCounter::Tick_Stopped);
        m_lo_counter.set_tick_source(TimerCounter::Tick_Timer);
        m_hi_counter.set_tick_source(TimerCounter::Tick_Timer);
    } else {
        m_sgl_counter.set_tick_source(m_timer_block ? TimerCounter::Tick_External :
                                                      TimerCounter::Tick_Timer);
        m_lo_counter.set_tick_source(TimerCounter::Tick_Stopped);
        m_hi_counter.set_tick_source(TimerCounter::Tick_Stopped);
    }
}


/*
 * Update the settings (top value, compare channel values, slope mode) of the counter
 * Single mode only
 */
void ArchXT_TimerA::configure_single_counter()
{
    m_sgl_counter.update();

    //Configure the top value, it's the PER register value except for FREQ mode which uses CMP0
    m_sgl_counter.set_top(m_wgmode == TCA_SINGLE_WGMODE_FRQ_gc ? m_cmp[0].value : m_per.value);

    //Set the compare channel value and reset the output if the waveform mode is NORMAL
    for (int i = 0; i < CFG::CompareChannelCount; ++i) {
        m_sgl_counter.set_comp_value(i, m_cmp[i].value);

        if (m_wgmode == TCA_SINGLE_WGMODE_NORMAL_gc)
            set_compare_output(i, Output_Clear);
    }

    //Select the slope mode according to the waveform mode
    TimerCounter::SlopeMode sm;
    switch (m_wgmode) {

        //Single slope mode: force up-counting
        case TCA_SINGLE_WGMODE_SINGLESLOPE_gc:
            sm = TimerCounter::Slope_Up;
            break;

        //Dual slope modes
        case TCA_SINGLE_WGMODE_DSTOP_gc:
        case TCA_SINGLE_WGMODE_DSBOTTOM_gc:
        case TCA_SINGLE_WGMODE_DSBOTH_gc:
            sm = TimerCounter::Slope_Double;
            break;

    //For all other modes, keep the current direction but ensure the dual slope is disabled
    default:
        sm = m_sgl_counter.countdown() ? TimerCounter::Slope_Down : TimerCounter::Slope_Up;
    }
    m_sgl_counter.set_slope_mode(sm);

    m_sgl_counter.reschedule();
}


/*
 * Update the timer_block boolean value, depending on the EventInput states (A and B)
 */
void ArchXT_TimerA::update_timer_block(uint8_t ev_ctrl)
{
    bool eva_enable = ev_ctrl & TCA_SINGLE_CNTAEI_bm;
    uint8_t eva_mode = ev_ctrl & TCA_SINGLE_EVACTA_gm;

    bool evb_enable;
    uint8_t evb_mode;
    if (m_config.version >= CFG::V2) {
        evb_enable = ev_ctrl & TCA_SINGLE_CNTBEI_bm;
        evb_mode = ev_ctrl & TCA_SINGLE_EVACTB_gm;
    } else {
        evb_enable = false;
        evb_mode = 0;
    }

    if (eva_enable) {
        if (evb_enable && evb_mode == TCA_SINGLE_EVACTB_RESTART_HIGHLVL_gc)
            m_timer_block = m_EIB_state;
        else if (eva_mode == TCA_SINGLE_EVACTA_CNT_HIGHLVL_gc)
            m_timer_block = !m_EIA_state;
        else if (eva_mode == TCA_SINGLE_EVACTA_UPDOWN_gc)
            m_timer_block = false;
        else
            m_timer_block = true;
    } else {
        m_timer_block = false;
    }
}


/*
 * Set the counting direction and associated register bits.
 * Only for single mode.
 */
void ArchXT_TimerA::set_direction(bool countdown, bool do_reschedule)
{
    //If the waveform mode is SingleSlope, force the counting to be up.
    if (m_wgmode == TCA_SINGLE_WGMODE_SINGLESLOPE_gc)
        countdown = false;

   //Update the counting direction, surround by update/reschedule if required.
    if (do_reschedule)
        m_sgl_counter.update();

    m_sgl_counter.set_countdown(countdown);

    if (do_reschedule)
        m_sgl_counter.reschedule();

    //Update the register bits.
    WRITE_IOREG_B(CTRLESET, TCA_SINGLE_DIR, countdown);
    WRITE_IOREG_B(CTRLECLR, TCA_SINGLE_DIR, countdown);
}


/*
 * Update the waveform output for a given compare channel.
 * It reads the corresponding CMPxEN and CMPxOV register bits and
 * raise the signal with the changed value: 0, 1 or Invalid (to indicate disabled)
 * argument: change should be one of the OutputChange enum values
 */
void ArchXT_TimerA::set_compare_output(int index, int change)
{
    vardata_t sig_value;
    if (m_split_mode || index < CFG::CompareChannelCount) {
        uint8_t ov_bit = index_to_CMPxOV_bit(m_split_mode, index);
        uint8_t en_bit = index_to_CMPxEN_bit(m_split_mode, index);

        uint8_t old_value = test_ioreg(REG_ADDR(CTRLC), ov_bit);
        uint8_t new_value;
        switch(change) {
            case Output_Set:    new_value = 1;             break;
            case Output_Clear:  new_value = 0;             break;
            case Output_Toggle: new_value = 1 - old_value; break;
            default:            new_value = old_value;
        }

        write_ioreg(REG_ADDR(CTRLC), ov_bit, new_value);

        //Conditions to have the waveform output enabled:
        // - the corresponding CMPxEN bit is set and,
        // - split mode, or in single mode, the waveform generation mode is not NORMAL and,
        // - the peripheral is counting clock ticks, not events (CNTAEI == 0)
        if (test_ioreg(REG_ADDR(CTRLB), en_bit) &&
            (m_split_mode || m_wgmode != TCA_SINGLE_WGMODE_NORMAL_gc) &&
            !TEST_IOREG(EVCTRL, TCA_SINGLE_CNTAEI))

            sig_value = vardata_t(new_value);
    }

    if (sig_value != m_signal.data(Signal_CompareOutput, index))
        m_signal.raise(Signal_CompareOutput, sig_value, index);
}


void ArchXT_TimerA::update_compare_outputs(int change)
{
    for (int i = 0; i < CFG::CompareChannelCount * 2; ++i)
        set_compare_output(i, change);
}


void ArchXT_TimerA::raised(const signal_data_t& sigdata, int hooktag)
{
    if (hooktag == Tag_Single)
        process_counter_single(sigdata);
    else if (hooktag == Tag_SplitLow)
        process_counter_split(sigdata, true);
    else
        process_counter_split(sigdata, false);
}


void ArchXT_TimerA::process_counter_single(const signal_data_t& sigdata)
{
    if (sigdata.sigid == TimerCounter::Signal_Event) {
        int event_flags = sigdata.data.as_uint();

        //The counter reaches the TOP value
        if (event_flags & TimerCounter::Event_Top) {
            switch (m_wgmode) {
                //On normal and frequency modes, when counting up, update the buffered registers
                //and raise the OVF flag
                case TCA_SINGLE_WGMODE_NORMAL_gc:
                case TCA_SINGLE_WGMODE_FRQ_gc:
                    if (!m_sgl_counter.countdown()) {
                        update_buffered_registers();
                        m_ovf_intflag.set_flag();
                    }
                    break;

                //For all dual slope modes
                case TCA_SINGLE_WGMODE_DSTOP_gc:
                case TCA_SINGLE_WGMODE_DSBOTTOM_gc:
                case TCA_SINGLE_WGMODE_DSBOTH_gc:
                    //The counter changes direction so update the register bits
                    SET_IOREG(CTRLESET, TCA_SINGLE_DIR);
                    SET_IOREG(CTRLECLR, TCA_SINGLE_DIR);
                    //In DSBOTH and DSTOP modes, raise the OVF flag
                    if (m_wgmode == TCA_SINGLE_WGMODE_DSTOP_gc || m_wgmode == TCA_SINGLE_WGMODE_DSBOTH_gc)
                        m_ovf_intflag.set_flag();
                    break;

            }
        }

        //The counter reaches the BOTTOM value
        if (event_flags & TimerCounter::Event_Bottom) {
            switch (m_wgmode) {

                //On normal and frequency modes, when counting down, update the buffered registers
                //and raise the OVF flag
                case TCA_SINGLE_WGMODE_NORMAL_gc:
                case TCA_SINGLE_WGMODE_FRQ_gc:
                    if (m_sgl_counter.countdown()) {
                        update_buffered_registers();
                        m_ovf_intflag.set_flag();
                    }
                    break;

                //For single slope mode
                case TCA_SINGLE_WGMODE_SINGLESLOPE_gc:
                    //Buffered registers update
                    update_buffered_registers();
                    //if the compare values are not zero, they are set
                    for (int i = 0; i < CFG::CompareChannelCount; ++i)
                        set_compare_output(i, m_sgl_counter.comp_value(i) ? Output_Set : Output_Clear);
                    break;

                //For all dual slope modes
                case TCA_SINGLE_WGMODE_DSTOP_gc:
                case TCA_SINGLE_WGMODE_DSBOTTOM_gc:
                case TCA_SINGLE_WGMODE_DSBOTH_gc:
                    //Buffered registers update
                    update_buffered_registers();
                    //The counter changes direction so update the register bits
                    CLEAR_IOREG(CTRLESET, TCA_SINGLE_DIR);
                    CLEAR_IOREG(CTRLECLR, TCA_SINGLE_DIR);
                    //In DSBOTH and DSBOTTOM modes, raise the OVF flag
                    if (m_wgmode == TCA_SINGLE_WGMODE_DSBOTTOM_gc || m_wgmode == TCA_SINGLE_WGMODE_DSBOTH_gc)
                        m_ovf_intflag.set_flag();
                    //if the compare values are not zero, they are set
                    for (int i = 0; i < CFG::CompareChannelCount; ++i)
                        set_compare_output(i, m_sgl_counter.comp_value(i) ? Output_Set : Output_Clear);
                    break;

            }
        }
    }

    //Compare match event
    else if (sigdata.sigid == TimerCounter::Signal_CompMatch) {
        //Set the corresponding interrupt flag
        m_cmp_intflags[sigdata.index].set_flag();

        switch (m_wgmode) {
            //In frequency mode, the corresponding waveform output is toggled
            case TCA_SINGLE_WGMODE_FRQ_gc:
                set_compare_output(sigdata.index, Output_Toggle);
                break;

                //In single slope mode, the corresponding waveform output is cleared
            case TCA_SINGLE_WGMODE_SINGLESLOPE_gc:
                set_compare_output(sigdata.index, Output_Clear);
                break;

            //In any dual slope mode, the corresponding waveform output is cleared
            //when counting up and set when counting down
            case TCA_SINGLE_WGMODE_DSTOP_gc:
            case TCA_SINGLE_WGMODE_DSBOTTOM_gc:
            case TCA_SINGLE_WGMODE_DSBOTH_gc:
                set_compare_output(sigdata.index, m_sgl_counter.countdown() ? Output_Set : Output_Clear);
                break;

        }
    }
}


void ArchXT_TimerA::process_counter_split(const signal_data_t& sigdata, bool low_cnt)
{
    if (sigdata.sigid == TimerCounter::Signal_Event) {
        int event_flags = sigdata.data.as_uint();

        //The counter reaches the BOTTOM value
        if (event_flags & TimerCounter::Event_Bottom) {
            if (low_cnt) {
                //OVF vector is also known as LUNF
                m_ovf_intflag.set_flag();
                //Reset the waveform outputs for the lo counter
                for (int i = 0; i < CFG::CompareChannelCount; ++i)
                    set_compare_output(i, m_lo_counter.comp_value(i) ? Output_Set : Output_Clear);

            } else {
                m_hunf_intflag.set_flag();
                //Reset the waveform outputs for the high counter
                for (int i = 0; i < CFG::CompareChannelCount; ++i)
                    set_compare_output(i + CFG::CompareChannelCount, m_hi_counter.comp_value(i) ? Output_Set : Output_Clear);
            }
        }
    }

    //Compare match event
    else if (sigdata.sigid == TimerCounter::Signal_CompMatch) {
        if (low_cnt) {
            //Set the corresponding interrupt flag
            m_cmp_intflags[sigdata.index].set_flag();
            //Set the waveform output for the corresponding channel
            set_compare_output(sigdata.index, Output_Set);
        } else {
            //No compare match interrupt flag for the high counter
            //Set the waveform output for the corresponding channel
            set_compare_output(sigdata.index + CFG::CompareChannelCount, Output_Set);
        }
    }
}


void ArchXT_TimerA::event_raised(const signal_data_t& sigdata, int hooktag)
{
    if (m_split_mode) return;

    if (hooktag == Hook_EventA)
        process_EIA(sigdata.data.as_uint());
    else if (hooktag == Hook_EventB && m_config.version >= CFG::V2)
        process_EIB(sigdata.data.as_uint());
}


/*
 * Process a signal on EventInputA
 */
void ArchXT_TimerA::process_EIA(bool event_state)
{
    //Check that the EIA state actually changes and store the new state
    if (event_state == m_EIA_state) return;
    m_EIA_state = event_state;

    //If the TCA is disabled, nothing to do
    if (!TEST_IOREG(CTRLA, TCA_SINGLE_ENABLE)) return;

    //If EventInputA is disabled, nothing to do
    uint8_t ev_ctrl = READ_IOREG(EVCTRL);
    if (!(ev_ctrl & TCA_SINGLE_CNTAEI_bm)) return;

    m_sgl_counter.update();

    //Update the timer_block value
    update_timer_block(ev_ctrl);

    //Process the event according to the register settings
    uint8_t eva_mode = ev_ctrl & TCA_SINGLE_EVACTA_gm;
    switch(eva_mode) {
        case TCA_SINGLE_EVACTA_CNT_POSEDGE_gc:
            if (event_state)
                m_sgl_counter.tick();
            break;

        case TCA_SINGLE_EVACTA_CNT_ANYEDGE_gc:
            m_sgl_counter.tick();
            break;

        case TCA_SINGLE_EVACTA_CNT_HIGHLVL_gc:
            m_sgl_counter.set_tick_source(m_timer_block ? TimerCounter::Tick_External:
                                                          TimerCounter::Tick_Timer);
            break;

        case TCA_SINGLE_EVACTA_UPDOWN_gc:
            //if EventInputB is enabled and also set to UPDOWN, do a OR operation
            if ((ev_ctrl & TCA_SINGLE_CNTBEI_bm) &&
                ((ev_ctrl & TCA_SINGLE_EVACTB_gm) == TCA_SINGLE_EVACTB_UPDOWN_gc))
                event_state |= m_EIB_state;

            set_direction(event_state, false);
            break;
    }

    m_sgl_counter.reschedule();
}


void ArchXT_TimerA::process_EIB(bool event_state)
{
    if (event_state == m_EIB_state) return;
    m_EIB_state = event_state;

    if (TEST_IOREG(CTRLA, TCA_SINGLE_ENABLE)) return;

    uint8_t ev_ctrl = READ_IOREG(EVCTRL);

    if (!(ev_ctrl & TCA_SINGLE_CNTBEI_bm)) return;

    m_sgl_counter.update();

    update_timer_block(ev_ctrl);

    uint8_t evb_mode = ev_ctrl & TCA_SINGLE_EVACTB_gm;
    switch (evb_mode) {
        case TCA_SINGLE_EVACTB_UPDOWN_gc:
            //if EventInputA is enabled and also set to UPDOWN, do a OR operation
            if ((ev_ctrl & TCA_SINGLE_CNTAEI_bm) &&
                ((ev_ctrl & TCA_SINGLE_EVACTA_gm) == TCA_SINGLE_EVACTA_UPDOWN_gc))
                event_state |= m_EIA_state;

            set_direction(event_state, false);
            break;

        case TCA_SINGLE_EVACTB_RESTART_POSEDGE_gc:
            if (event_state)
                execute_command(TCA_SINGLE_CMD_RESTART_gc, true);
            break;

        case TCA_SINGLE_EVACTB_RESTART_ANYEDGE_gc:
            execute_command(TCA_SINGLE_CMD_RESTART_gc, true);
            break;

        case TCA_SINGLE_EVACTB_RESTART_HIGHLVL_gc:
            m_sgl_counter.set_tick_source(m_timer_block ? TimerCounter::Tick_External:
                                                          TimerCounter::Tick_Timer);
            if (m_EIB_state)
                execute_command(TCA_SINGLE_CMD_RESTART_gc, true);
            break;
    }

    m_sgl_counter.reschedule();
}


void ArchXT_TimerA::sleep(bool on, SleepMode mode)
{
    //For version>=2, in standby mode, keep running if the RUNSTDBY is set.
    //The peripheral always sleeps for modes above Standby
    bool do_sleep = ( mode == SleepMode::Standby &&
                      !(m_config.version >= CFG::V2 && TEST_IOREG(CTRLA, TCA_SINGLE_RUNSTDBY))
                    ) || mode > SleepMode::Standby;

    m_timer.set_paused(do_sleep and on);
}


bool ArchXT_TimerA::execute_command(int cmd, bool cmden)
{
    if (!cmden) return true;

    switch(cmd) {
        case TCA_SINGLE_CMD_UPDATE_gc:
            update_buffered_registers();
            return true;

        case TCA_SINGLE_CMD_RESET_gc:
            reset();

            //Explicit reset of the control registers
            WRITE_IOREG(CTRLA, 0);
            WRITE_IOREG(CTRLB, 0);
            WRITE_IOREG(CTRLC, 0);
            WRITE_IOREG(CTRLESET, 0);
            WRITE_IOREG(CTRLECLR, 0);
            WRITE_IOREG(CTRLFSET, 0);
            WRITE_IOREG(CTRLFCLR, 0);
            WRITE_IOREG(INTCTRL, 0);
            WRITE_IOREG(INTFLAGS, 0);
            WRITE_IOREG(TEMP, 0);

            //Signal the reset of all Compare Output values
            update_compare_outputs(Output_Clear);

            return false;

        case TCA_SINGLE_CMD_RESTART_gc:
            if (m_split_mode) {
                m_lo_counter.update();
                m_lo_counter.set_counter(0);
                m_lo_counter.reschedule();

                m_hi_counter.update();
                m_hi_counter.set_counter(0);
                m_hi_counter.reschedule();

            } else {

                m_sgl_counter.update();
                m_sgl_counter.set_counter(0);
                set_direction(false, false);
                m_sgl_counter.reschedule();
            }
            //Clear all Compare Output values
            WRITE_IOREG(CTRLC, 0);
            update_compare_outputs(Output_Clear);

            return false;

        default:
            return true;
    }
}
