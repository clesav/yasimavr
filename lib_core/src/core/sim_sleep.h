/*
 * sim_sleep.h
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

#ifndef __YASIMAVR_SLEEP_H__
#define __YASIMAVR_SLEEP_H__

#include "sim_peripheral.h"
#include "sim_signal.h"
#include "sim_types.h"


//=======================================================================================
/*
 * CTLREQ definitions
*/

//AVR_CTLREQ_SLEEP_CALL (defined in sim_peripheral.h) :
//Request sent by the CPU when executing a SLEEP instruction.
//No data is required

//AVR_CTLREQ_SLEEP_PSEUDO (defined in sim_peripheral.h) :
//Request sent by the CPU when executing a infinite loop (RJMP .-2) with GIE flag set
//No data is required

/*
 *Definition of generic sleep modes
*/
enum class AVR_SleepMode {
	Invalid,
	Active,
	Pseudo,
	Idle,
	ADC,
	Standby,
	ExtStandby,
	PowerDown,
	PowerSave,
};

const char* SleepModeName(AVR_SleepMode mode);


//=======================================================================================
/*
 * Generic sleep mode controller
 * On receiving a sleep request, it checks the I/O registers that sleep is enabled and
 * which mode to enter. It then notifies the Device by a AVR_CTLREQ_CORE_SLEEP request.
 * The sleep controller connects to the interrupt controller to get notified of any
 * interrupt raised. It filters the notifications depending on the selected sleep mode
 * and if the device should be woken up, sends a AVR_CTLREQ_CORE_WAKEUP request to
 * the Device
*/

struct AVR_SleepConfig {

	struct mode_config_t : base_reg_config_t {
		AVR_SleepMode mode;
		//Bitset that indicates for each vector of the device vector map
		//if the corresponding interrupt can wake up the device from this sleep mode
		uint8_t int_mask[16];
	};

	std::vector<mode_config_t> modes;
	regbit_t rb_mode;
	regbit_t rb_enable;
};


class DLL_EXPORT AVR_SleepController : public AVR_Peripheral, public AVR_SignalHook {

public:

	AVR_SleepController(const AVR_SleepConfig& config);

	virtual bool init(AVR_Device& device) override;
	virtual bool ctlreq(uint16_t req, ctlreq_data_t *data) override;
	virtual void raised(const signal_data_t& data, uint16_t __unused) override;

private:

	const AVR_SleepConfig& m_config;
	//Index of the current sleep mode in the configuration mode map
	uint8_t m_mode_index;

};

#endif //__YASIMAVR_SLEEP_H__
