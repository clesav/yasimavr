/*
 * sim_port.cpp
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

#include "sim_port.h"
#include "../core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

/**
   Constructor of the port controller.
   \param name Upper case letter identifying the port. (eg. 'A' denotes port PA)
 */
Port::Port(char name)
:Peripheral(AVR_IOCTL_PORT(name))
,m_name(name)
,m_pinmask(0)
,m_pins(8)
,m_port_value(0)
{}


bool Port::init(Device& device)
{
    bool status = Peripheral::init(device);

    //Construct the pin mask by looking up for pins names
    //Pcx where c is the port letter and x is 0 to 7
    char pinname[4];
    m_pinmask = 0;
    for (int i = 0; i < 8; ++i) {
        std::sprintf(pinname, "P%c%d", m_name, i);
        Pin *pin = device.find_pin(pinname);
        if (pin) {
            pin->signal().connect(*this, i);
            m_pinmask |= (1 << i);
        }
        m_pins[i] = pin;
    }

    return status;
}


void Port::reset()
{
    //On reset, we set the internal state of all the pins to floating
    uint8_t pinmask = m_pinmask;
    for (int i = 0; i < 8; ++i) {
        if (pinmask & 1)
            m_pins[i]->set_gpio_state(Pin::State_Floating);
        pinmask >>= 1;
    }

    m_port_value = 0;
    m_signal.raise(0, m_port_value);
}


bool Port::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_signal;
        return true;
    }
    return false;
}


/**
   Set the pin internal state and raise the signal.
   \param num index of the pin (0 to 7)
   \param state new state for the pin
   \sa pin_state_changed
 */
void Port::set_pin_internal_state(uint8_t num, Pin::State state)
{
    if (num < 8 && ((m_pinmask >> num) & 1)) {
        logger().dbg("Pin %d set to %s", num, Pin::StateName(state));
        m_pins[num]->set_gpio_state(state);
    }
}


void Port::raised(const signal_data_t& sigdata, int hooktag)
{
    if (sigdata.sigid == Pin::Signal_StateChange) {
        Pin::State pin_state = (Pin::State) sigdata.data.as_int();
        uint8_t pin_num = hooktag;
        pin_state_changed(pin_num, pin_state);
    }
}


/**
   Callback method called when the resolved state of a pin has changed.
   \sa set_pin_internal_state
 */
void Port::pin_state_changed(uint8_t num, Pin::State state)
{
    logger().dbg("Detected Pin %d change to %s", num, Pin::StateName(state));
    if (state == Pin::State_Shorted) {
        ctlreq_data_t d = { .data = m_pins[num]->id() };
        device()->ctlreq(AVR_IOCTL_CORE, AVR_CTLREQ_CORE_SHORTING, &d);
        return;
    }

    if (m_pins[num]->digital_state())
        m_port_value |= 1 << num;
    else
        m_port_value &= ~(1 << num);

    m_signal.raise(0, m_port_value);
}
