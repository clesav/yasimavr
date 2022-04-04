/*
 * sim_port.h
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

#ifndef __YASIMAVR_IO_PORT_H__
#define __YASIMAVR_IO_PORT_H__

#include "../core/sim_peripheral.h"
#include "../core/sim_types.h"
#include "../core/sim_pin.h"


//=======================================================================================
/*
 * AVR_IO_Port implements a GPIO port controller for up to 8 bits
 * the exact number of pins is determined by the device configuration
 * An initialization, the port will lookup all possible ports with the letter
 * (e.g. port 'B' will lookup all pins named 'PBx' (x=0 to 7)
 */
class DLL_EXPORT AVR_IO_Port : public AVR_Peripheral, public AVR_SignalHook {

public:

	//Constructor of the port. The name is the upper case letter identifying the port.
	AVR_IO_Port(char name);

	//Implementation of AVR_Peripheral callbacks
	virtual bool init(AVR_Device& device) override;
	virtual void reset() override;
    virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void raised(const signal_data_t& sigdata, uint16_t sigid) override;

protected:

	//returns the pin mask, containing a '1' for each existing pin
    uint8_t pin_mask() const;
	//Set the internal state of a pin (0 to 7)
    void set_pin_internal_state(uint8_t num, AVR_Pin::State state);
	//Callback method called when the resolved state of a pin has changed
	//The default implementation only handles the SHORTED case and the logging
    virtual void pin_state_changed(uint8_t num, AVR_Pin::State state);

private:

    const char m_name;
    uint8_t m_pinmask;
	AVR_Signal m_signal;
	std::vector<AVR_Pin*> m_pins;
	uint8_t m_port_value;

};

inline uint8_t AVR_IO_Port::pin_mask() const
{
	return m_pinmask;
}

#endif //__YASIMAVR_IO_PORT_H__
