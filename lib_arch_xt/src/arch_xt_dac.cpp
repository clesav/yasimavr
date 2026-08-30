/*
 * arch_xt_dac.cpp
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

#include "arch_xt_dac.h"
#include "avr_io/io_dac.h"
#include "core/sim_sleep.h"
#include "core/sim_device.h"
#include "ioctrl_common/sim_vref.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(DAC_t, reg))


ArchXT_DAC::ArchXT_DAC(int num, const ArchXT_DACConfig& config)
:Peripheral(AVR_IOCTL_DAC(0x30 + num))
,m_config(config)
,m_vref_signal(nullptr)
,m_vref_hook(*this, &ArchXT_DAC::vref_sig_raised)
,m_sleeping(false)
{
    m_signal.raise(Signal_Output, 0.0);
}


bool ArchXT_DAC::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), DAC_ENABLE_bm | DAC_OUTEN_bm | DAC_RUNSTDBY_bm);
    add_ioreg(REG_ADDR(DATA));

    m_vref_signal = dynamic_cast<DataSignal*>(get_signal(AVR_IOCTL_VREF));
    if (m_vref_signal)
        m_vref_signal->connect(m_vref_hook);
    else
        status = false;

    return status;
}


void ArchXT_DAC::reset(int)
{
    m_sleeping = false;
    update_output();
}


bool ArchXT_DAC::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }

    return false;
}


void ArchXT_DAC::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //If enabling the peripheral, check that VCC is set
    if (addr == REG_ADDR(CTRLA) && (data.posedge() & DAC_ENABLE_bm)) {
        if (!m_vref_signal->data(VREF::Signal_VCCChange).as_double()) {
            device()->crash(CRASH_INVALID_CONFIG, "VCC voltage not set");
            return;
        }
    }

    update_output();
}


void ArchXT_DAC::update_output()
{
    double dac_value;
    if (TEST_IOREG(CTRLA, DAC_ENABLE) && !m_sleeping) {
        vardata_t vref = m_vref_signal->data(VREF::Signal_IntRefChange, m_config.vref_channel);
        dac_value = vref.as_double() * READ_IOREG(DATA) / 256.0;
    } else {
        dac_value = 0.0;
    }

    logger().dbg("Updating output to %f", dac_value);
    m_signal.raise(Signal_Output, dac_value);
}


void ArchXT_DAC::vref_sig_raised(const signal_data_t& sigdata, int)
{
    //Update the output value in case of a change of internal reference
    if (sigdata.sigid == VREF::Signal_IntRefChange && sigdata.index == m_config.vref_channel)
        update_output();
}


void ArchXT_DAC::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !TEST_IOREG(CTRLA, DAC_RUNSTDBY))) {
        m_sleeping = on;
        update_output();
    }
}
