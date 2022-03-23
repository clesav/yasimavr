/*
 * arch_avr_extint.h
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

#ifndef __YASIMAVR_AVR_EXTINT_H__
#define __YASIMAVR_AVR_EXTINT_H__

#include "core/sim_peripheral.h"
#include "core/sim_interrupt.h"
#include "core/sim_pin.h"
#include "core/sim_types.h"


//=======================================================================================

#define EXTINT_PIN_COUNT		2
#define PCINT_PIN_COUNT			24
#define PCINT_BANK_COUNT		(PCINT_PIN_COUNT / 8)


//=======================================================================================
/*
 * Implementation of a External Interrupt controller for AVR series
 */
 
struct AVR_ArchAVR_ExtIntConfig {

	uint32_t extint_pins[EXTINT_PIN_COUNT];
	uint32_t pcint_pins[PCINT_PIN_COUNT];
	regbit_t rb_extint_ctrl;
	regbit_t rb_extint_mask;
	regbit_t rb_extint_flag;
	regbit_t rb_pcint_ctrl;
	regbit_t rb_pcint_flag;
	reg_addr_t reg_pcint_mask[PCINT_BANK_COUNT];
	int_vect_t extint_vector[EXTINT_PIN_COUNT];
	int_vect_t pcint_vector[PCINT_BANK_COUNT];

};


class DLL_EXPORT AVR_ArchAVR_ExtInt : public AVR_Peripheral,
									  public AVR_InterruptHandler,
									  public AVR_SignalHook {

public:

	enum SignalId {
		Signal_ExtInt,
		Signal_PinChange
	};

	AVR_ArchAVR_ExtInt(const AVR_ArchAVR_ExtIntConfig& config);

	virtual bool init(AVR_Device& device) override;
	virtual void reset() override;
	virtual bool ctlreq(uint16_t req, ctlreq_data_t* data) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void interrupt_ack_handler(int_vect_t vector) override;

    virtual void raised(const signal_data_t& sigdata, uint16_t hooktag) override;

private:

    const AVR_ArchAVR_ExtIntConfig& m_config;
	//Signals that get raised when an external interrupt condition is detected
    AVR_Signal m_signal;
	//Backup copies of pin states to detect edges
    uint8_t m_extint_pin_value;
    uint8_t m_pcint_pin_value[PCINT_BANK_COUNT];

    uint8_t get_extint_mode(uint8_t pin) const;

};

#endif //__YASIMAVR_AVR_EXTINT_H__
