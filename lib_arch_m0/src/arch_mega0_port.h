/*
 * arch_mega0_port.h
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

#ifndef __YASIMAVR_MEGA0_PORT_H__
#define __YASIMAVR_MEGA0_PORT_H__

#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_port.h"


//=======================================================================================
/*
 * Implementation of a GPIO port controller for Mega0/1 series, based on the generic
 * AVIO_IO_Port class
 */
 
struct AVR_ArchMega0_PortConfig {

	reg_addr_t reg_base_port;
	reg_addr_t reg_base_vport;
	int_vect_t iv_port;
};


class DLL_EXPORT AVR_ArchMega0_Port : public AVR_IO_Port, public AVR_InterruptHandler {

public:

	AVR_ArchMega0_Port(char name, const AVR_ArchMega0_PortConfig& config);

	virtual bool init(AVR_Device& device) override;
	virtual void reset() override;
	virtual void ioreg_read_handler(reg_addr_t addr) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;

protected:

    virtual void pin_state_changed(uint8_t num, AVR_Pin::State state) override;

private:

    const AVR_ArchMega0_PortConfig& m_config;
	uint8_t m_port_value;
	uint8_t m_dir_value;

	void update_pin_states();
};

#endif //__YASIMAVR_MEGA0_PORT_H__
