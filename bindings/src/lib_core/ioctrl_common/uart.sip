/*
 * uart.sip
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

#include "ioctrl_common/sim_uart.h"

const uint16_t CTLREQ_UART_ENDPOINT = AVR_CTLREQ_UART_ENDPOINT;

%End

const uint16_t CTLREQ_UART_ENDPOINT;


enum UART_SignalIndex /BaseType=IntEnum/ {
	UART_Data_Frame,
	UART_Data_String,
	UART_TX_Start,
	UART_TX_Complete,
	UART_RX_Start,
	UART_RX_Complete,
};

struct UART_EndPoint {
%TypeHeaderCode
#include "ioctrl_common/sim_uart.h"
%End

	AVR_Signal* tx_signal;
	AVR_SignalHook* rx_hook;

};


class AVR_IO_UART : public AVR_SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_uart.h"
%End

public:

	AVR_IO_UART();
	virtual ~AVR_IO_UART();

	void init(AVR_CycleManager&, AVR_DeviceLogger&);
	void reset();
    
	AVR_Signal& signal();
	void set_frame_delay(cycle_count_t);
	void set_tx_buffer_limit(unsigned int);
	void push_tx(uint8_t);
	void cancel_tx_pending();
	unsigned int tx_pending() const;
	bool has_tx_collision() const;
	void clear_tx_collision();
	
	void set_rx_buffer_limit(unsigned int);
	void set_rx_enabled(bool);
	uint8_t rx_available() const;
	uint8_t pop_rx();
	bool has_rx_overflow() const;
	void clear_rx_overflow();

};
