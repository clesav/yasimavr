/*
 * sim_uart.h
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

#ifndef __YASIMAVR_UART_H__
#define __YASIMAVR_UART_H__

#include "../core/sim_types.h"
#include "../core/sim_cycle_timer.h"
#include "../core/sim_signal.h"
#include "../core/sim_logger.h"
#include <deque>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_uart Universal Asynchronous Serial Interface framework
   @{
 */

/**
   \name Controller requests definition for USART
   @{
 */

/**
   adds frame to the underlying RX buffer, bypassing the bus logic. For debugging purpose.
 */
#define AVR_CTLREQ_USART_BYTES        (AVR_CTLREQ_BASE + 1)


/// @}
/// @}


//=======================================================================================

namespace UART {

enum SignalId {
    /// Raised at the start of a frame transmission, with sigdata containing the frame.
    Signal_TX_Start,
    /// Raised at the end of a frame transmission.
    /// sigdata contains 1 if the transmission completed successfully or 0 if it was interrupted.
    Signal_TX_Complete,
    /// Raised at the start of a frame reception.
    Signal_RX_Start,
    /// Raised at the end of a frame reception.
    /// sigdata contains 1 if the frame is received successfully or 0 if it was discarded.
    Signal_RX_Complete,
    /// Raised on transmission of a frame. sigdata contains the raw frame transmitted.
    Signal_TX_Frame,
    /// Raised on transmission of a frame. sigdata contains the data transmitted.
    Signal_TX_Data,
    /// Raised when the RX buffer overruns.
    Signal_RX_Overflow,
    /// Raised when the TX buffer overruns.
    Signal_TX_Collision,
};

enum ClockMode {
    /// Asynchronous mode, the clock line is not used.
    Clock_Async,
    /// Synchronous mode where the interface is the clock master.
    Clock_Emitter,
    /// Synchronous mode where the interface is not the clock master.
    Clock_Receiver,
};

enum Line {
    Line_TXD,
    Line_RXD,
    Line_XCK,
    Line_DIR,
};

enum Parity {
    Parity_No,
    Parity_Odd,
    Parity_Even,
};


/**
   \ingroup api_uart
   \brief Generic model defining an universal synchronous/asynchronous serial interface a.k.a. USART

   \par Emitter
   The TX part is composed of a FIFO, whose front slot is the shift register
   push_tx() puts a new 8-bits frame into the FIFO and the transmission will start
   immediately. If a TX is already in progress, the frame will wait until it can
   be transmitted. If the TX buffer size reached the limit, the most recently pushed
   frames will be discarded and the collision flag will be set.
   Frames are sent via signaling, using both UART_Data_Frame and UART_TX_Start.
   At the end of transmission, a signal UART_TX_Complete is emitted with data = 1
   if successful or 0 if canceled mid-way by a reset.
   On-going TX can only be canceled by a reset.

   \par Receiver
   The RX part is composed of a FIFO with two sub-parts: Received frames and pending frames (yet to
   be received);
   Disabling the RX does not prevent receiving frames. They are simply discarded when actually
   received by the device. (i.e. when moved from the pending FIFO to the received FIFO)
   The signal UART_RX_Start is emitted at the start of a reception.
   The signal UART_RX_Complete are emitted at the end of a reception, with data = 1 if the frame
   if kept or data = 0 if canceled or discarded.
 */
class AVR_CORE_PUBLIC_API USART {

public:

    USART();
    virtual ~USART() = default;

    void init(CycleManager& cycle_manager, Logger& logger);

    void reset();

    Signal& signal();

    void set_clock_mode(ClockMode mode);
    void set_bit_delay(cycle_count_t delay);

    void set_stopbits(unsigned short count);
    void set_databits(unsigned short count);
    void set_parity(Parity parity);

    void set_tx_buffer_limit(size_t limit);
    void push_tx(uint16_t frame);
    void cancel_tx_pending();
    size_t tx_pending() const;
    void set_tx_dir_enabled(bool enabled);
    bool tx_in_progress() const;

    void set_rx_buffer_limit(size_t limit);
    void set_rx_enabled(bool enabled);
    size_t rx_available() const;
    void pop_rx();
    uint16_t read_rx() const;
    bool has_frame_error() const;
    bool has_parity_error() const;
    bool has_rx_overrun() const;
    bool rx_in_progress() const;

    void push_rx_frame(uint16_t frame);

    void set_paused(bool enabled);

    uint16_t build_frame(uint16_t data) const;
    uint16_t parse_frame(uint16_t frame) const;

    void line_state_changed(Line line, bool new_state);

protected:

    virtual void set_line_state(Line line, bool state) = 0;
    virtual bool get_line_state(Line line) const = 0;

private:

    CycleManager* m_cycle_manager;
    Logger* m_logger;

    Signal m_signal;

    //=================================
    //Clock management
    cycle_count_t m_delay;      //Bit duration in clock cycles
    ClockMode m_clk_mode;
    BoundFunctionCycleTimer<USART> m_clk_timer;

    unsigned short m_databits;
    unsigned short m_stopbits;
    Parity m_parity;

    uint16_t m_tx_shifter;
    unsigned short m_tx_shift_counter;
    std::deque<uint16_t> m_tx_buffer;
    //Size limit for the TX FIFO, including the shift register
    size_t m_tx_limit;
    //Collision flag
    //bool m_tx_collision;
    bool m_tx_dir_enabled;
    //Cycle timer to simulate the delay to emit a frame
    BoundFunctionCycleTimer<USART> m_tx_timer;

    //Enable/disable flag for RX
    bool m_rx_enabled;
    uint16_t m_rx_shifter;
    unsigned short m_rx_shift_counter;
    //RX FIFO buffers
    std::deque<uint16_t> m_rx_buffer;
    std::deque<uint16_t> m_rx_pending;
    //Size limit for the received part of the RX FIFO
    //The pending part of the FIFO is not limited
    size_t m_rx_limit;
    //Cycle timer to simulate the delay to receive a frame
    BoundFunctionCycleTimer<USART> m_rx_timer;

    //Pause flag for both RX and TX
    bool m_paused;

    unsigned short framesize() const;

    void fill_tx_shifter();
    void start_bitwise_tx();
    void shift_tx();

    void start_bitwise_rx();
    void shift_rx();

    cycle_count_t clk_timer_next(cycle_count_t when);
    cycle_count_t tx_timer_next(cycle_count_t when);
    cycle_count_t rx_timer_next(cycle_count_t when);

    void process_clock_change(bool new_state);

};

/// Getter for the internal signal used for operation signaling.
inline Signal& USART::signal()
{
    return m_signal;
}

/// Getter for the number of frames stored in the RX buffer.
inline size_t USART::rx_available() const
{
    return m_rx_buffer.size();
}

/// Getter for the no of frames waiting in the buffer to be emitted.
inline size_t USART::tx_pending() const
{
    return m_tx_buffer.size() ? (m_tx_buffer.size() - 1) : 0;
}


}; //namespace UART


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_UART_H__
