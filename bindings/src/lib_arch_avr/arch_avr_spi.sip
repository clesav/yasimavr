/*
 * arch_avr_spi.sip
 *
 *	Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

struct AVR_ArchAVR_SPI_Config {
%TypeHeaderCode
#include "arch_avr_spi.h"
%End

	reg_addr_t reg_data;

	regbit_t rb_enable;
	regbit_t rb_int_enable;
	regbit_t rb_int_flag;
	regbit_t rb_mode;
	
	uint32_t pin_select;
	
	regbit_t rb_clock;
	regbit_t rb_clock2x;

	int_vect_t iv_spi;

};


class AVR_ArchAVR_SPI : public AVR_Peripheral, public AVR_SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_spi.h"
%End

public:

	AVR_ArchAVR_SPI(uint8_t num, const AVR_ArchAVR_SPI_Config&);

	virtual bool init(AVR_Device&);
	virtual void reset();
	virtual bool ctlreq(uint16_t, ctlreq_data_t*);
	virtual void ioreg_read_handler(reg_addr_t);
	virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
	virtual void raised(const signal_data_t&, uint16_t);

};