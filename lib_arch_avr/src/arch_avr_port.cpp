/*
 * arch_avr_port.cpp
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

#include "arch_avr_port.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================
/*
 * Constructor of a GPIO port
 */
ArchAVR_Port::ArchAVR_Port(const ArchAVR_PortConfig& config)
:Port(config.name)
,m_config(config)
,m_portr_value(0)
,m_ddr_value(0)
{}

/*
 * Initialisation of a GPIO port
 */
bool ArchAVR_Port::init(Device& device)
{
    bool status = Port::init(device);

    add_ioreg(m_config.reg_port, pin_mask());
    add_ioreg(m_config.reg_pin, pin_mask());
    add_ioreg(m_config.reg_dir, pin_mask());

    return status;
}

/*
 * Reset of a GPIO port. The reset of the pins is done by the device
 */
void ArchAVR_Port::reset()
{
    Port::reset();

    m_portr_value = 0;
    m_ddr_value = 0;
}

void ArchAVR_Port::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.reg_port) {
        update_pin_states(data.value, m_ddr_value);
        m_portr_value = data.value;
    }
    else if (addr == m_config.reg_dir) {
        update_pin_states(m_portr_value, data.value);
        m_ddr_value = data.value;
    }
    //Writing a 1 to PINR toggles the pin state
    else if (addr == m_config.reg_pin) {
        uint8_t port_value = read_ioreg(m_config.reg_port) ^ (data.value & pin_mask());
        write_ioreg(m_config.reg_port, port_value);
        update_pin_states(port_value, m_ddr_value);
        m_portr_value = data.value;
    }
}

void ArchAVR_Port::update_pin_states(uint8_t portr, uint8_t ddr)
{
    //Filters the pins to update to only those for which portr or ddr has actually changed
    uint8_t pinmask = ((portr ^ m_portr_value) | (ddr ^ m_ddr_value)) & pin_mask();
    Pin::State state;
    for (int i = 0; i < 8; ++i) {
        if (pinmask & 1) {
            if (ddr & 1) {
                state = portr & 1 ? Pin::State_High : Pin::State_Low;
            } else {
                state = portr & 1 ? Pin::State_PullUp : Pin::State_Floating;
            }
            set_pin_internal_state(i, state);
        }
        pinmask >>= 1;
        portr >>= 1;
        ddr >>= 1;
    }
}

/*
 * Callback for a state change of a pin. Update PINR with the new value
 */
void ArchAVR_Port::pin_state_changed(uint8_t num, Pin::State state)
{
    Port::pin_state_changed(num, state);
    //The SHORTED case is taken care of by Port
    if (state != Pin::State_Shorted) {
        bool new_value = pin(num)->digital_state();
        write_ioreg(m_config.reg_pin, num, new_value);
    }
}
