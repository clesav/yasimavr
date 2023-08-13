/*
 * arch_xt_misc.cpp
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

#include "arch_xt_misc.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"
#include <cstring>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define VREF_REG_ADDR(reg) \
    (m_config.reg_base + offsetof(VREF_t, reg))


ArchXT_VREF::ArchXT_VREF(const ArchXT_VREFConfig& config)
:VREF(config.channels.size())
,m_config(config)
{}

bool ArchXT_VREF::init(Device& device)
{
    bool status = VREF::init(device);

    add_ioreg(VREF_REG_ADDR(CTRLB));

    for (auto channel: m_config.channels)
        add_ioreg(channel.rb_select);

    return status;
}

void ArchXT_VREF::reset()
{
    //Set each reference channel to the reset value
    for (unsigned int index = 0; index < m_config.channels.size(); ++index)
        set_channel_reference(index, 0);
}

void ArchXT_VREF::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //Iterate over all the channels, and update if impacted by the register change
    for (unsigned int ch_ix = 0; ch_ix < m_config.channels.size(); ++ch_ix) {
        const ArchXT_VREFConfig::channel_t& ch = m_config.channels[ch_ix];
        if (addr == ch.rb_select.addr && ch.rb_select.extract(data.anyedge())) {
            //Extract the selection value for this channel
            uint8_t reg_value = ch.rb_select.extract(data.value);
            set_channel_reference(ch_ix, reg_value);
        }
    }
}

void ArchXT_VREF::set_channel_reference(unsigned int index, uint8_t reg_value)
{
    typedef ArchXT_VREFConfig::reference_config_t vref_cfg_t;

    //Find the corresponding reference setting from the configuration
    auto vref_cfg = find_reg_config_p<vref_cfg_t>(m_config.channels[index].references, reg_value);
    //If it's a valid setting, update the reference
    if (vref_cfg)
        set_reference(index, vref_cfg->source, vref_cfg->level);
}


//=======================================================================================

enum InterruptPriority {
    IntrPriorityLevel0 = 0,
    IntrPriorityLevel1 = 1
};

#define INT_REG_ADDR(reg) \
    (m_config.reg_base + offsetof(CPUINT_t, reg))

ArchXT_IntCtrl::ArchXT_IntCtrl(const ArchXT_IntCtrlConfig& config)
:InterruptController(config.vector_count)
,m_config(config)
{}

bool ArchXT_IntCtrl::init(Device& device)
{
    bool status = InterruptController::init(device);

    //CTRLA not implemented
    add_ioreg(INT_REG_ADDR(STATUS));
    add_ioreg(INT_REG_ADDR(LVL0PRI));
    add_ioreg(INT_REG_ADDR(LVL1VEC));

    return status;
}

void ArchXT_IntCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    update_irq();
}

void ArchXT_IntCtrl::cpu_ack_irq(int_vect_t vector)
{
    //When a interrupt is acked (its routine is about to be executed),
    //set the corresponding priority level flag
    if (vector > 0) {
        uint8_t priority_bit = (vector == read_ioreg(INT_REG_ADDR(LVL1VEC))) ?
                                IntrPriorityLevel1 : IntrPriorityLevel0;
        set_ioreg(INT_REG_ADDR(STATUS), priority_bit);
    }

    InterruptController::cpu_ack_irq(vector);

    set_interrupt_raised(vector, true);
}

int_vect_t ArchXT_IntCtrl::get_next_irq() const
{
    uint8_t status_ex = read_ioreg(INT_REG_ADDR(STATUS));

    if (BITSET(status_ex, IntrPriorityLevel1))
        return AVR_INTERRUPT_NONE;

    int lvl1_vector = read_ioreg(INT_REG_ADDR(LVL1VEC));
    if (lvl1_vector > 0 && interrupt_raised(lvl1_vector))
        return lvl1_vector;

    if (BITSET(status_ex, IntrPriorityLevel0))
        return AVR_INTERRUPT_NONE;

    int lvl0_vector = read_ioreg(INT_REG_ADDR(LVL0PRI));
    if (!lvl0_vector) {
        for (int_vect_t i = 0; i < intr_count(); ++i) {
            if (interrupt_raised(i))
                return i;
        }
    } else {
        for (int_vect_t i = 1; i <= intr_count(); ++i) {
            int_vect_t v = (i + lvl0_vector) % intr_count();
            if (interrupt_raised(v))
                return v;
        }
    }

    return AVR_INTERRUPT_NONE;
}

void ArchXT_IntCtrl::cpu_reti()
{
    //The priority level flag must be cleared
    uint8_t status_ex = read_ioreg(INT_REG_ADDR(STATUS));
    if (BITSET(status_ex, IntrPriorityLevel1)) {
        clear_ioreg(INT_REG_ADDR(STATUS), IntrPriorityLevel1);
    } else {
        clear_ioreg(INT_REG_ADDR(STATUS), IntrPriorityLevel0);
    }

    InterruptController::cpu_reti();
}

//=======================================================================================

#define RST_REG_ADDR(reg) \
    reg_addr_t(m_base_reg + offsetof(RSTCTRL_t, reg))

/*
 * Constructor of a reset controller
 */
ArchXT_ResetCtrl::ArchXT_ResetCtrl(reg_addr_t base)
:Peripheral(AVR_IOCTL_RST)
,m_base_reg(base)
,m_rst_flags(0)
{}

bool ArchXT_ResetCtrl::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(RST_REG_ADDR(RSTFR));
    add_ioreg(RST_REG_ADDR(SWRR));

    return status;
}

void ArchXT_ResetCtrl::reset()
{
    //Request the value of the reset flags from the device and set the bits of the
    //register RSTFR accordingly
    ctlreq_data_t reqdata;
    if (device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET_FLAG, &reqdata)) {
        int flags = reqdata.data.as_int();
        if (flags & Device::Reset_BOD)
            m_rst_flags |= RSTCTRL_BORF_bm;
        if (flags & Device::Reset_WDT)
            m_rst_flags |= RSTCTRL_WDRF_bm;
        if (flags & Device::Reset_Ext)
            m_rst_flags |= RSTCTRL_EXTRF_bm;
        if (flags & Device::Reset_SW)
            m_rst_flags |= RSTCTRL_SWRF_bm;
        //On a Power On reset, all the other reset flag bits must be cleared
        if (flags & Device::Reset_PowerOn)
            m_rst_flags = RSTCTRL_PORF_bm;

        write_ioreg(RST_REG_ADDR(RSTFR), m_rst_flags);
    }
}

void ArchXT_ResetCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == RST_REG_ADDR(RSTFR)) {
        //Clears the flags to which '1' is written
        m_rst_flags &= ~data.value;
        write_ioreg(RST_REG_ADDR(RSTFR), m_rst_flags);
    }
    else if (addr == RST_REG_ADDR(SWRR)){
        //Writing a '1' to SWRE bit triggers a software reset
        if (data.value & RSTCTRL_SWRE_bm) {
            ctlreq_data_t reqdata = { .data = Device::Reset_SW };
            device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);
        }
    }
}


//=======================================================================================

#define SIGROW_REG_ADDR(reg) \
    (m_config.reg_base_sigrow + offsetof(SIGROW_t, reg))

#define SIGROW_MEM_OFS      3
#define SIGROW_MEM_SIZE     (sizeof(SIGROW_t) - SIGROW_MEM_OFS)

ArchXT_MiscRegCtrl::ArchXT_MiscRegCtrl(const ArchXT_MiscConfig& config)
:Peripheral(chr_to_id('M', 'I', 'S', 'C'))
,m_config(config)
{
    m_sigrow = (uint8_t*) malloc(SIGROW_MEM_SIZE);
    memset(m_sigrow, 0x00, SIGROW_MEM_SIZE);
}

ArchXT_MiscRegCtrl::~ArchXT_MiscRegCtrl()
{
    free(m_sigrow);
}

bool ArchXT_MiscRegCtrl::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(CCP);

    for (unsigned int i = 0; i < m_config.gpior_count; ++i)
        add_ioreg(m_config.reg_base_gpior + i);

    add_ioreg(m_config.reg_revid, 0xFF, true);

    add_ioreg(SIGROW_REG_ADDR(DEVICEID0), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(DEVICEID1), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(DEVICEID2), 0xFF, true);

    for (int i = 0; i < 10; ++i)
        add_ioreg(SIGROW_REG_ADDR(SERNUM0) + i, 0xFF, true);

    add_ioreg(SIGROW_REG_ADDR(OSCCAL16M0), 0x7F, true);
    add_ioreg(SIGROW_REG_ADDR(OSCCAL20M1), 0x0F, true);
    add_ioreg(SIGROW_REG_ADDR(OSCCAL16M0), 0x7F, true);
    add_ioreg(SIGROW_REG_ADDR(OSCCAL20M1), 0x0F, true);
    add_ioreg(SIGROW_REG_ADDR(TEMPSENSE0), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(TEMPSENSE1), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(OSC16ERR3V), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(OSC16ERR5V), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(OSC20ERR3V), 0xFF, true);
    add_ioreg(SIGROW_REG_ADDR(OSC20ERR5V), 0xFF, true);

    return status;
}

void ArchXT_MiscRegCtrl::reset()
{
    write_ioreg(m_config.reg_revid, MCU_REVID);
    write_ioreg(SIGROW_REG_ADDR(DEVICEID0), m_config.dev_id & 0xFF);
    write_ioreg(SIGROW_REG_ADDR(DEVICEID1), (m_config.dev_id >> 8) & 0xFF);
    write_ioreg(SIGROW_REG_ADDR(DEVICEID2), (m_config.dev_id >> 16) & 0xFF);
}

bool ArchXT_MiscRegCtrl::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_WRITE_SIGROW) {
        memcpy(m_sigrow, data->data.as_ptr(), SIGROW_MEM_SIZE);
        return true;
    }
    return false;
}

uint8_t ArchXT_MiscRegCtrl::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    if (addr >= m_config.reg_base_sigrow &&
        addr < reg_addr_t(m_config.reg_base_sigrow + sizeof(SIGROW_t))) {

        reg_addr_t reg_ofs = addr - m_config.reg_base_sigrow;
        if (reg_ofs >= SIGROW_MEM_OFS)
            value = m_sigrow[reg_ofs - SIGROW_MEM_OFS];
    }

    return value;
}

void ArchXT_MiscRegCtrl::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == CCP) {
        if (data.value == CCP_SPM_gc || data.value == CCP_IOREG_gc) {
            const char* mode_name = (data.value == CCP_SPM_gc) ? "SPM" : "IOREG";
            logger().dbg("Configuration Control Protection inhibited, mode = %s", mode_name);
            write_ioreg(addr, 0x00);
        }
    }
}
