/*
 * interrupt.sip
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

%ModuleCode

#include "core/sim_interrupt.h"
const uint16_t CTLREQ_INTR_REGISTER = AVR_CTLREQ_INTR_REGISTER;
const int16_t INTERRUPT_NONE = -1;

%End

const uint16_t CTLREQ_INTR_REGISTER;
const int16_t INTERRUPT_NONE;


//=======================================================================================

class AVR_InterruptController : public AVR_Peripheral /Abstract/ {
%TypeHeaderCode
#include "core/sim_interrupt.h"
%End

public:

	enum SignalId /BaseType=IntEnum/ {
		Signal_Raised			/PyName=Raised/,
		Signal_Cancelled		/PyName=Cancelled/,
		Signal_Acknowledged		/PyName=Acknowledged/,
		Signal_Returned			/PyName=Returned/
	};
	
	AVR_InterruptController(unsigned int size);
	virtual ~AVR_InterruptController();
	
	virtual void reset();
	virtual bool ctlreq(uint16_t, ctlreq_data_t*);
	virtual void sleep(bool, AVR_SleepMode);

};


//=======================================================================================

class AVR_InterruptHandler {
%TypeHeaderCode
#include "core/sim_interrupt.h"
%End

public:

	AVR_InterruptHandler();
	virtual ~AVR_InterruptHandler();

	void raise_interrupt(int_vect_t vector) const;
	void cancel_interrupt(int_vect_t vector) const;
	virtual void interrupt_ack_handler(int_vect_t vector);

};


//=======================================================================================

class AVR_InterruptFlag : public AVR_InterruptHandler /NoDefaultCtors/ {
%TypeHeaderCode
#include "core/sim_interrupt.h"
%End

public:

	AVR_InterruptFlag(bool);
	
	bool init(AVR_Device&, const regbit_t&, const regbit_t&, int_vect_t);
	int update_from_ioreg();
	bool set_flag(uint8_t = 0xFF);
	bool clear_flag(uint8_t = 0xFF);
	bool raised() const;
	virtual void interrupt_ack_handler(int_vect_t);

};