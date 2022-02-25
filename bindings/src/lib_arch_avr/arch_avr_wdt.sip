/*
 * arch_avr_wdt.sip
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

struct AVR_ArchAVR_WDT_Config {
%TypeHeaderCode
#include "arch_avr_wdt.h"
%End

	uint32_t clock_frequency;
	UnsignedInt32Vector delays;
	
	reg_addr_t reg_wdt;
	bitmask_t bm_delay;
	bitmask_t bm_chg_enable;
	bitmask_t bm_reset_enable;
	bitmask_t bm_int_enable;
	bitmask_t bm_int_flag;
	regbit_t rb_reset_flag;
	
	int_vect_t vector;

};

class AVR_ArchAVR_WDT : public AVR_WatchdogTimer,
                        public AVR_InterruptHandler
                        /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_wdt.h"
%End

public:

	AVR_ArchAVR_WDT(const AVR_ArchAVR_WDT_Config&);

	virtual bool init(AVR_Device&);
	virtual void reset();
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
	virtual void interrupt_ack_handler(int_vect_t);

protected:

	virtual void timeout();

};
