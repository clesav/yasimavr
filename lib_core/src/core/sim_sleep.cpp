/*
 * sim_sleep.cpp
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

#include "sim_sleep.h"
#include "sim_device.h"
#include "sim_interrupt.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

static const char* SleepModeNames[] = {
    "Invalid",
    "Active",
    "Pseudo",
    "Idle",
    "ADC",
    "Standby",
    "Extended Standby",
    "Power Down",
    "Power Save",
};

const char* YASIMAVR_QUALIFIED_NAME(SleepModeName)(SleepMode mode)
{
    return SleepModeNames[(int) mode];
}


SleepController::SleepController(const SleepConfig& config)
:Peripheral(AVR_IOCTL_SLEEP)
,m_config(config)
,m_mode_index(0)
{}

bool SleepController::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rb_mode);
    add_ioreg(m_config.rb_enable);

    Signal* s = get_signal(AVR_IOCTL_INTR);
    if (s)
        s->connect(*this);
    else
        status = false;

    return status;
}

bool SleepController::ctlreq(ctlreq_id_t req, ctlreq_data_t*)
{
    //On a Sleep request (from a SLEEP instruction),
    // 1 - check that the sleep controller is enabled
    // 2 - check that the mode register is set to a configured sleep mode
    // 3 - send the sleep request to the device
    if (req == AVR_CTLREQ_SLEEP_CALL) {
        if (test_ioreg(m_config.rb_enable)) {
            uint8_t reg_mode_value = read_ioreg(m_config.rb_mode);
            int index = find_reg_config<SleepConfig::mode_config_t>(m_config.modes, reg_mode_value);
            if (index >= 0) {
                SleepMode mode = m_config.modes[index].mode;
                if (mode >= SleepMode::Idle) {
                    m_mode_index = index;
                    ctlreq_data_t d = { .data = (int) mode };
                    device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SLEEP, &d);
                }
            }
        }
        return true;
    }

    //On a Pseudo sleep request, check if the option is enabled and
    //send the sleep request to the device
    else if (req == AVR_CTLREQ_SLEEP_PSEUDO) {
        if (!device()->test_option(Device::Option_DisablePseudoSleep)) {
            ctlreq_data_t d = { .data = (int) SleepMode::Pseudo };
            device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SLEEP, &d);
        }
        return true;
    }

    return false;
}

void SleepController::raised(const signal_data_t& sigdata, int)
{
    //data.u contains the state of the interrupt. We only do something on a raised interrupt
    //identifiable by the bit 0
    if (sigdata.sigid != InterruptController::Signal_StateChange || !(sigdata.data.as_int() & 0x01))
        return;

    int_vect_t vector = sigdata.index;
    SleepMode sleep_mode = device()->sleep_mode();
    bool do_wake_up;
    //In Pseudo sleep mode, any interrupt wakes up the device
    if (sleep_mode == SleepMode::Pseudo) {
        do_wake_up = true;
    }
    //For any actual sleep mode, extract the flag from the configuration bitset to know
    //whether the raised interrupt can wake up or not the device
    else if (sleep_mode >= SleepMode::Idle) {
        uint8_t num_byte = vector / 8;
        uint8_t num_bit = vector % 8;
        do_wake_up = (m_config.modes[m_mode_index].int_mask[num_byte] >> num_bit) & 0x01;
    }
    //Remaining cases are Invalid and Active so do nothing
    else {
        do_wake_up = false;
    }

    if (do_wake_up) {
        const char* mode_name = SleepModeName(sleep_mode);
        logger().dbg("Waking device from mode %s on vector %d", mode_name, vector);
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_WAKEUP, nullptr);
    }
}
