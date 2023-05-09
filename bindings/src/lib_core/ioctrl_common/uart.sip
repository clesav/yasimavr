/*
 * uart.sip
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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


//=======================================================================================

const uint16_t CTLREQ_UART_ENDPOINT;


struct UART_EndPoint {
%TypeHeaderCode
#include "ioctrl_common/sim_uart.h"
%End

    Signal* tx_signal;
    SignalHook* rx_hook;

};


class IO_UART : public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_uart.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_DataFrame        /PyName=DataFrame/,
        Signal_DataString       /PyName=DataString/,
        Signal_DataBytes        /PyName=DataBytes/,
        Signal_TX_Start         /PyName=TX_Start/,
        Signal_TX_Complete      /PyName=TX_Complete/,
        Signal_RX_Start         /PyName=RX_Start/,
        Signal_RX_Complete      /PyName=RX_Complete/,
    };

    IO_UART();
    virtual ~IO_UART();

    void init(CycleManager&, Logger&);
    void reset();

    Signal& signal();
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

    void set_paused(bool);

};
