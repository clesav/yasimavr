/*
 * uart.sip
 *
 *  Copyright 2022-2025 Clement Savergne <csavergne@yahoo.com>

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

const ctlreq_id_t CTLREQ_USART_BYTES = AVR_CTLREQ_USART_BYTES;

%End


//=======================================================================================

const ctlreq_id_t CTLREQ_USART_BYTES;


namespace UART {
%TypeHeaderCode
#include "ioctrl_common/sim_uart.h"
%End

enum SignalId /BaseType=IntEnum/ {
    Signal_TX_Start         /PyName=TX_Start/,
    Signal_TX_Complete      /PyName=TX_Complete/,
    Signal_RX_Start         /PyName=RX_Start/,
    Signal_RX_Complete      /PyName=RX_Complete/,
    Signal_TX_Frame         /PyName=TX_Frame/,
    Signal_TX_Data          /PyName=TX_Data/,
    Signal_RX_Overflow      /PyName=RX_Overflow/,
    Signal_TX_Collision     /PyName=TX_Collision/,
};

enum ClockMode {
    Clock_Async             /PyName=Async/,
    Clock_Emitter           /PyName=Emitter/,
    Clock_Receiver          /PyName=Receiver/,
};

enum Line /BaseType=IntEnum/ {
    Line_TXD                /PyName=TXD/,
    Line_RXD                /PyName=RXD/,
    Line_XCK                /PyName=XCK/,
    Line_DIR                /PyName=DIR/,
};

enum Parity {
    Parity_No               /PyName=No/,
    Parity_Odd              /PyName=Odd/,
    Parity_Even             /PyName=Even/,
};

class USART /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_uart.h"
%End

public:

    USART();

    void init(CycleManager&, Logger*=nullptr);
    void reset();

    Signal& signal();

    void set_clock_mode(ClockMode);
    void set_bit_delay(cycle_count_t);

    void set_stopbits(unsigned short);
    void set_databits(unsigned short);
    void set_parity(Parity);

    void set_tx_buffer_limit(size_t);
    void push_tx(uint16_t);
    void cancel_tx_pending();
    size_t tx_pending() const;
    void set_tx_dir_enabled(bool);
    bool tx_in_progress() const;

    void set_rx_buffer_limit(size_t);
    void set_rx_enabled(bool);
    size_t rx_available() const;
    void pop_rx();
    uint16_t read_rx() const;
    bool has_frame_error() const;
    bool has_parity_error() const;
    bool has_rx_overrun() const;
    bool rx_in_progress() const;
    void push_rx_frame(uint16_t);

    void set_paused(bool);

    uint16_t build_frame(uint16_t) const;
    uint16_t parse_frame(uint16_t) const;

    void line_state_changed(Line, bool);

protected:

    virtual void set_line_state(Line, bool) = 0;
    virtual bool get_line_state(Line) const = 0;

};

};
