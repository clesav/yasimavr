/*
 * arch_avr_wdt.cpp
 *
 *  Copyright 2021-2026 Clement Savergne <csavergne@yahoo.com>

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
#include "arch_avr_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_WDT::ArchAVR_WDT(const ArchAVR_WDTConfig& config)
:Peripheral(AVR_IOCTL_WDT)
,m_config(config)
,m_wdt_timer(*this, &ArchAVR_WDT::wdt_timeout)
,m_lock_timer(*this, &ArchAVR_WDT::lock_timeout)
{}


bool ArchAVR_WDT::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.reg_wdt);

    status &= register_interrupt(m_config.iv_wdt, *this);

    m_wdt_timer.init(*device.cycle_manager());
    m_lock_timer.init(*device.cycle_manager());

    return status;
}


void ArchAVR_WDT::reset(int flags)
{
    m_wdt_timer.cancel();
    m_lock_timer.cancel();

    //Check if the watchdog reset flag is set. If it is, WDE is forced to 1
    //and the watchdog timer is activated with default delay settings.
    ctlreq_data_t reqdata;
    if (flags & Device::Reset_WDT) {
        set_ioreg(m_config.reg_wdt, m_config.bs_reset_enable);
        reschedule_timer();
    }
}


/*
 * Handle the watchdog reset request by rescheduling the timer
 */
bool ArchAVR_WDT::ctlreq(ctlreq_id_t req, ctlreq_data_t*)
{
    if (req == AVR_CTLREQ_WATCHDOG_RESET) {
        if (m_wdt_timer.scheduled())
            reschedule_timer();
        return true;
    }
    return false;
}


void ArchAVR_WDT::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    bitmask_t bm_wdp;
    for (auto& rb : m_config.rbc_delay)
        bm_wdp |= rb;

    uint8_t reg_value = data.value;
    //Forces WDE to be set if WDRF is set
    if (test_ioreg(m_config.rb_reset_flag))
        reg_value |= m_config.bs_reset_enable;

    //If the register is unlocked, force WDCE to be set
    if (m_lock_timer.scheduled()) {
        reg_value |= m_config.bs_chg_enable;
    } else {
        //If the register is locked,
        //Prevent any change to WDP
        reg_value = bm_wdp.replace(reg_value, data.old);
        //Prevent clearing WDE
        if (data.negedge() & m_config.bs_reset_enable)
            reg_value |= m_config.bs_reset_enable;
    }
    write_ioreg(m_config.reg_wdt, reg_value);
    ioreg_write_t data2 = { reg_value, data.old };

    if (m_lock_timer.scheduled()) {
        //Update the prescaler setting
        if (data2.anyedge() & bm_wdp)
            reschedule_timer();

        if (data2.negedge() & m_config.bs_reset_enable) {
            m_wdt_timer.cancel();
            logger().dbg("Watchdog timer stopped");
        }

    } else {

        //If the register is locked, we can unlock it with WDCE=1 and WDE=1, start
        //the unlock timed sequence
        //However, the unlocking only has effect on the next write
        if ((m_config.bs_chg_enable & reg_value) && (m_config.bs_reset_enable & reg_value)) {
            m_lock_timer.delay(4);
            logger().dbg("Register unlocked");
        }
    }

    if (data2.posedge() & (m_config.bs_reset_enable | m_config.bs_int_enable))
        reschedule_timer();

    //If WDIF is written to 1 by the CPU, we clear it
    if (reg_value & m_config.bs_int_flag)
        clear_ioreg(m_config.reg_wdt, m_config.bs_int_flag);
}


cycle_count_t ArchAVR_WDT::calculate_delay()
{
    uint8_t reg_value = read_ioreg(m_config.rbc_delay);
    unsigned long ps_factor = (reg_value <= 9) ? 1 << (reg_value + 11) : 0;
    cycle_count_t delay = (device()->frequency() * ps_factor) / m_config.clock_frequency;
    return delay;
}


void ArchAVR_WDT::reschedule_timer()
{
    cycle_count_t delay = calculate_delay();
    m_wdt_timer.delay(delay);
    logger().dbg("Next timeout scheduled, PS factor = %lu", delay);
}


void ArchAVR_WDT::wdt_timeout()
{
    logger().dbg("timeout");

    uint8_t reg_value = read_ioreg(m_config.reg_wdt);

    //If the interrupt is enabled but not raised yet, raise it.
    //If WDE is also set, restart the timer
    if ((reg_value & m_config.bs_int_enable) && !(reg_value & m_config.bs_int_flag)) {
        logger().dbg("Raising interrupt");
        set_ioreg(m_config.reg_wdt, m_config.bs_int_flag);
        raise_interrupt(m_config.iv_wdt);

        //Reschedule the timer for the next interrupt/reset
        logger().dbg("Next timeout scheduled");
        m_wdt_timer.delay(calculate_delay());
    }
    //or else, WDE is set or WDIF is already raised so trigger the reset.
    //Don't call reset() itself because we want the current
    //cycle to complete beforehand. The state of the device would be
    //inconsistent otherwise.
    else if ((reg_value & m_config.bs_reset_enable) || (reg_value & m_config.bs_int_flag)) {
        logger().dbg("Triggering a device reset");
        ctlreq_data_t reqdata = { .data = Device::Reset_WDT };
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_RESET, &reqdata);

        //No need to reschedule the timer, the reset handler will do it
    }
}


bool ArchAVR_WDT::interrupt_ack_handler(int_vect_t vector)
{
    //Datasheet: "Executing the corresponding interrupt vector will clear WDIE and WDIF"
    clear_ioreg(m_config.reg_wdt, m_config.bs_int_flag);
    clear_ioreg(m_config.reg_wdt, m_config.bs_int_enable);
    reschedule_timer();
    return true;
}


void ArchAVR_WDT::lock_timeout()
{
    clear_ioreg(m_config.reg_wdt, m_config.bs_chg_enable);
}
