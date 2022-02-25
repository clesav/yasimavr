/*
 * ioreg.sip
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


struct ioreg_write_t {
%TypeHeaderCode
#include "core/sim_ioreg.h"
%End

	uint8_t value;
	uint8_t posedge;
	uint8_t negedge;
	uint8_t old;
};


class AVR_IO_RegHandler /Abstract/ {
%TypeHeaderCode
#include "core/sim_ioreg.h"
%End

public:

    virtual ~AVR_IO_RegHandler();
    
    virtual void ioreg_read_handler(reg_addr_t) = 0;
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&) = 0;

};


class AVR_IO_Register {
%TypeHeaderCode
#include "core/sim_ioreg.h"
%End

public:

	AVR_IO_Register(bool=false);
    ~AVR_IO_Register();

    uint8_t value() const;
    void set(uint8_t);

    void set_handler(AVR_IO_RegHandler*, uint8_t, uint8_t);

    uint8_t cpu_read(reg_addr_t);
    bool cpu_write(reg_addr_t, uint8_t);

    uint8_t ioctl_read(reg_addr_t);
    void ioctl_write(reg_addr_t, uint8_t);

};
