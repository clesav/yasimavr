/*
 * arch_avr_wdt.cpp
 *
 *  Copyright 2021-2024 Clement Savergne <csavergne@yahoo.com>

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
:Peripheral(AVR_IOCTL_WDT)
,m_config(config)
,m_timer_start_cycle(0)
,m_wdt_timer(*this, &ArchAVR_WDT::wdt_timeout)
,m_lock_timer(*this, &ArchAVR_WDT::lock_timeout)
{}


bool ArchAVR_WDT::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.reg_wdt);

    status &= register_interrupt(m_config.iv_wdt, *this);

    return status;
}


void ArchAVR_WDT::reset()
{
    device()->cycle_manager()->cancel(m_lock_timer);

    m_timer_start_cycle = device()->cycle();

    //Check if the watchdog reset flag is set. If it is, WDE is forced to 1
    //and the watchdog timer is activated with default delay settings.
    ctlreq_data_t reqdata;
    if (device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET_FLAG, &reqdata)) {
        if (reqdata.data.as_uint() & Device::Reset_WDT)
            set_ioreg(m_config.reg_wdt, m_config.bm_reset_enable);
    }

    reschedule_timer();
}


/*
 * Handle the watchdog reset request by rescheduling the timer
 */
bool ArchAVR_WDT::ctlreq(ctlreq_id_t req, ctlreq_data_t*)
{
    if (req == AVR_CTLREQ_WATCHDOG_RESET) {
        m_timer_start_cycle = device()->cycle();
        reschedule_timer();
        return true;
    }
    return false;
}


void ArchAVR_WDT::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    bool chg_enable = m_config.bm_chg_enable.extract(data.value);
    bool rst_enable = m_config.bm_reset_enable.extract(data.value);

    //Forces WDE to be set if WDRF is set
    if (!rst_enable && test_ioreg(m_config.rb_reset_flag)) {
        rst_enable = true;
        set_ioreg(m_config.reg_wdt, m_config.bm_reset_enable);
    }

    if (m_lock_timer.scheduled()) {
        //If the register is unlocked, force WDCE to be set
        if (!chg_enable) {
            set_ioreg(m_config.reg_wdt, m_config.bm_chg_enable);
            chg_enable = true;
        }
    } else {
        //If the register is locked, we can unlock it with WDCE=1 and WDE=1, start
        //the unlock timed sequence
        //However, the unlocking only has effect on the next write
        if (chg_enable && rst_enable)
            device()->cycle_manager()->delay(m_lock_timer, 4);
        //Restore the bit values for WDE and WDP
        write_ioreg(m_config.reg_wdt, m_config.bm_reset_enable, m_config.bm_reset_enable.extract(data.old));
        for (auto& rb : m_config.rbc_delay)
            write_ioreg(rb, rb.extract(data.old));
    }

    //If WDIF is written to 1 by the CPU, we clear it
    if (m_config.bm_int_flag.extract(data.value))
        clear_ioreg(m_config.reg_wdt, m_config.bm_int_flag);

    reschedule_timer();
}


cycle_count_t ArchAVR_WDT::calculate_timeout_delay()
{
    unsigned int ps_index = read_ioreg(m_config.rbc_delay);
    long long ps_factor = m_config.delays[ps_index];
    cycle_count_t timeout_cycles = (ps_factor * device()->frequency()) / m_config.clock_frequency;
    if (!timeout_cycles) timeout_cycles = 1;

    cycle_count_t elapsed_timeout_count = (device()->cycle() - m_timer_start_cycle) / timeout_cycles;
    cycle_count_t next_timeout_delay = (elapsed_timeout_count + 1) * timeout_cycles;

    logger().dbg("Timeout delay : %lld", next_timeout_delay);

    return next_timeout_delay;
}


void ArchAVR_WDT::reschedule_timer()
{
    if (test_ioreg(m_config.reg_wdt, m_config.bm_reset_enable) || test_ioreg(m_config.reg_wdt, m_config.bm_int_enable)) {
        cycle_count_t timeout_cycle = calculate_timeout_delay() + m_timer_start_cycle;
        if (timeout_cycle <= device()->cycle())
            timeout_cycle = device()->cycle() + 1;

        device()->cycle_manager()->schedule(m_wdt_timer, timeout_cycle);
        logger().dbg("Next timeout scheduled at : %lld", timeout_cycle);
    }
    else if (m_wdt_timer.scheduled()) {
        device()->cycle_manager()->cancel(m_wdt_timer);
    }
}


cycle_count_t ArchAVR_WDT::wdt_timeout(cycle_count_t when)
{
    logger().dbg("timeout");

    //If the interrupt is enabled but not raised yet, raise it.
    //If WDE is also set, restart the timer
    if (test_ioreg(m_config.reg_wdt, m_config.bm_int_enable) && !test_ioreg(m_config.reg_wdt, m_config.bm_int_flag)) {
        logger().dbg("Raising interrupt");
        set_ioreg(m_config.reg_wdt, m_config.bm_int_flag);
        raise_interrupt(m_config.iv_wdt);

        //Reschedule the timer for the next interrupt/reset
        m_timer_start_cycle = when;
        cycle_count_t timeout_cycle = calculate_timeout_delay() + when;
        logger().dbg("Next timeout scheduled at : %lld", timeout_cycle);
        return timeout_cycle;
    }
    //of else, WDE is set or WDIF is already raised so trigger the reset.
    //Don't call reset() itself because we want the current
    //cycle to complete beforehand. The state of the device would be
    //inconsistent otherwise.
    else {
        logger().dbg("Triggering a device reset");
        ctlreq_data_t reqdata = { .data = Device::Reset_WDT };
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);

        //No need to reschedule the timer, the reset handler will do it
        return 0;
    }
}


void ArchAVR_WDT::interrupt_ack_handler(int_vect_t vector)
{
    //Datasheet: "Executing the corresponding interrupt vector will clear WDIE and WDIF"
    clear_ioreg(m_config.reg_wdt, m_config.bm_int_flag);
    clear_ioreg(m_config.reg_wdt, m_config.bm_int_enable);
    reschedule_timer();
}


void ArchAVR_WDT::lock_timeout()
{
    clear_ioreg(m_config.reg_wdt, m_config.bm_chg_enable);
}
