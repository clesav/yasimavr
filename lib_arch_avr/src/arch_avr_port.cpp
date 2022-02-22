/*
 * arch_avr_port.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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


//=======================================================================================
/*
 * Constructor of a GPIO port
 */
AVR_ArchAVR_Port::AVR_ArchAVR_Port(const AVR_ArchAVR_PortConfig& config)
:AVR_IO_Port(config.name)
,m_config(config)
,m_portr_value(0)
,m_ddr_value(0)
{}

/*
 * Initialisation of a GPIO port
 */
bool AVR_ArchAVR_Port::init(AVR_Device& device)
{
	bool status = AVR_IO_Port::init(device);

	device.add_ioreg_handler(regbit_t(m_config.reg_port, 0, pin_mask()), this);
	device.add_ioreg_handler(regbit_t(m_config.reg_pin, 0, pin_mask()), this);
	device.add_ioreg_handler(regbit_t(m_config.reg_dir, 0, pin_mask()), this);

	return status;
}

/*
 * Reset of a GPIO port. The reset of the pins is done by the device
 */
void AVR_ArchAVR_Port::reset()
{
	AVR_IO_Port::reset();

	m_portr_value = 0;
	m_ddr_value = 0;
}

void AVR_ArchAVR_Port::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
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

void AVR_ArchAVR_Port::update_pin_states(uint8_t portr, uint8_t ddr)
{
	//Filters the pins to update to only those for which portr or ddr has actually changed
	uint8_t pinmask = ((portr ^ m_portr_value) | (ddr ^ m_ddr_value)) & pin_mask();
	AVR_Pin::State state;
	for (int i = 0; i < 8; ++i) {
		if (pinmask & 1) {
			if (ddr & 1) {
				state = portr & 1 ? AVR_Pin::State_High : AVR_Pin::State_Low;
			} else {
				state = portr & 1 ? AVR_Pin::State_PullUp : AVR_Pin::State_Floating;
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
void AVR_ArchAVR_Port::pin_state_changed(uint8_t num, AVR_Pin::State state)
{
	AVR_IO_Port::pin_state_changed(num, state);
	//The SHORTED case is taken care of by AVR_IO_Port
	if (state != AVR_Pin::State_Shorted) {
		bool curr_value = test_ioreg(m_config.reg_pin, num);
		bool new_value = (state == AVR_Pin::State_High ? 1 : 0);
		if (new_value ^ curr_value)
			write_ioreg(m_config.reg_pin, num, new_value);
	}
}
