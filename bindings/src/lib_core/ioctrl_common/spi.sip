/*
 * spi.sip
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

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

#include "ioctrl_common/sim_spi.h"

const ctlreq_id_t CTLREQ_SPI_TRANSFER = AVR_CTLREQ_SPI_TRANSFER;

%End

const ctlreq_id_t CTLREQ_SPI_TRANSFER;


//=======================================================================================

namespace SPI {
%TypeHeaderCode
#include "ioctrl_common/sim_spi.h"
%End

enum SerialMode {
	Mode0,
	Mode1,
	Mode2,
	Mode3,
};

enum BitOrder {
	MSBFirst,
	LSBFirst
};

enum Line /BaseType=IntEnum/ {
	Clock,
	MISO,
	MOSI,
	Select,
};


class EndPoint {
%TypeHeaderCode
#include "ioctrl_common/sim_spi.h"
%End

public:

    EndPoint();

    void set_serial_mode(SerialMode);
    SerialMode serial_mode() const;

    void set_bit_order(BitOrder);
    BitOrder bit_order() const;

    void set_shift_data(uint8_t);
    uint8_t shift_data() const;

    bool complete_frame() const;

protected:

    void set_active(bool);
    bool active() const;

    virtual void frame_completed();
    virtual void write_data_output(bool) = 0;
    virtual bool read_data_input() = 0;

    void set_shift_clock(bool);
    bool shift_clock() const;

};

}; //namespace SPI
