/*
 * arch_avr_wdp.cpp
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

#include "arch_avr_wdt.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_WDT::ArchAVR_WDT(const ArchAVR_WDTConfig& config)
:m_config(config)
,m_unlock_cycle(UINT64_MAX)
{}


bool ArchAVR_WDT::init(Device& device)
{
    bool status = WatchdogTimer::init(device);

    add_ioreg(m_config.reg_wdt);

    status &= register_interrupt(m_config.vector, *this);

    return status;
}


void ArchAVR_WDT::reset()
{
    //Check if the watchdog reset flag is set. If it is, WDE is forced to 1
    //and the watchdog timer is activated with default delay settings.
    ctlreq_data_t reqdata;
    if (device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET_FLAG, &reqdata)) {
        if (reqdata.data.as_uint() & Device::Reset_WDT) {
            set_ioreg(m_config.reg_wdt, m_config.bm_reset_enable);
            configure_timer(true, 0);
        }
    }
}


void ArchAVR_WDT::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    bool change_enable = m_config.bm_chg_enable.extract(data.value);
    bool rst_enable = m_config.bm_reset_enable.extract(data.value);
    cycle_count_t curr_cycle = device()->cycle();

    //Forces WDE to be set if WDRF is set
    if (!rst_enable && test_ioreg(m_config.rb_reset_flag)) {
        rst_enable = true;
        set_ioreg(m_config.reg_wdt, m_config.bm_reset_enable);
    }

    //Reconfigure the watchdog cycle timer
    //On the condition that we're within 4 cycles after setting WDCE and that WDCE=0
    if (curr_cycle > m_unlock_cycle && curr_cycle <= (m_unlock_cycle + 4)) {
        if (!change_enable) {
            bool int_enable = m_config.bm_int_enable.extract(data.value);
            uint8_t delay_index = m_config.bm_delay.extract(data.value);
            configure_timer(rst_enable || int_enable, delay_index);
        }
        m_unlock_cycle = UINT64_MAX;
    }
    //If the register is locked, we can unlock it with WDCE=1 and WDE=1
    else if (change_enable && rst_enable) {
        m_unlock_cycle = curr_cycle;
    }

    //If WDIF is written to 1 by the CPU, we clear it
    if (m_config.bm_int_flag.extract(data.value))
        clear_ioreg(m_config.reg_wdt, m_config.bm_int_flag);
}


void ArchAVR_WDT::configure_timer(bool enable, uint8_t delay_index)
{
    if (enable) {
        unsigned long clk_factor = device()->frequency() / m_config.clock_frequency;
        unsigned long delay = m_config.delays[delay_index];
        set_timer(0, delay, clk_factor);
    } else {
        set_timer(0, 0, 0);
    }
}


void ArchAVR_WDT::timeout()
{
    uint8_t reg_value = read_ioreg(m_config.reg_wdt);
    bool rst_enable = m_config.bm_reset_enable.extract(reg_value);
    bool int_enable = m_config.bm_int_enable.extract(reg_value);
    bool int_flag = m_config.bm_int_flag.extract(reg_value);

    //If the interrupt is enabled but not raised yet, raise it
    //It clears the int enable automatically and, if WDE is also set,
    //restart the timer
    if (int_enable && !int_flag) {
        set_ioreg(m_config.reg_wdt, m_config.bm_int_flag);
        raise_interrupt(m_config.vector);
        if (rst_enable) {
            uint8_t delay_index = m_config.bm_delay.extract(reg_value);
            configure_timer(rst_enable, delay_index);
        }
    }
    //Trigger the reset. Don't call reset() itself because we want the current
    //cycle to complete beforehand. The state of the device would be
    //inconsistent otherwise.
    else {
        ctlreq_data_t reqdata = { .data = Device::Reset_WDT };
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);
    }
}


void ArchAVR_WDT::interrupt_ack_handler(int_vect_t vector)
{
    //Datasheet: "Executing the corresponding interrupt vector will clear WDIE and WDIF"
    clear_ioreg(m_config.reg_wdt, m_config.bm_int_flag);
    clear_ioreg(m_config.reg_wdt, m_config.bm_int_enable);
}
