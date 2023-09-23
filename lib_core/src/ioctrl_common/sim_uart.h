/*
 * sim_uart.h
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

#ifndef __YASIMAVR_UART_H__
#define __YASIMAVR_UART_H__

#include "../core/sim_types.h"
#include "../core/sim_device.h"
#include "../core/sim_signal.h"
#include <deque>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_uart Universal Asynchronous Serial Interface framework
   @{
 */

/**
   \name Controller requests definition for UART
   @{
 */

/**
   Request that can be used by external code to access the end point of a UART interface
   which can then be used to send & receive data via signaling both ways.\n
   data is set to point to the UARTEndPoint structure to connect to.
 */
#define AVR_CTLREQ_UART_ENDPOINT        1


/**
   Structure exchanged with CTLREQ_UART_ENDPOINT
 */
struct UARTEndPoint {

    Signal* tx_signal;
    SignalHook* rx_hook;

};

/// @}
/// @}


//=======================================================================================
/**
   \ingroup api_uart
   \brief Generic model defining an universal asynchronous serial interface a.k.a. UART

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
   The RX part is composed of a FIFO with two sub-parts:
   The front part is the actual device FIFO, from which received frames are read and popped.
   The back part has the frames yet to be received by the device. This is
   a convenient system that allows to send a whole string to the device in one signal, while
   the device will still receive the characters one by one with a proper timing.
   Disabling the RX does not prevent receiving frames. They are simply discarded when actually
   received by the device. (i.e. when moved from the back FIFO to the front FIFO)
   Frames are received when signaled with UART_Data_Frame or UART_Data_String.
   The signal UART_RX_Start is emitted at the start of a reception.
   The signal UART_RX_Complete are emitted at the end of a reception, with data = 1 if the frame
   if kept or data = 0 if canceled or discarded.
 */
class AVR_CORE_PUBLIC_API UART : public SignalHook {

public:

    enum SignalId {
        /// Raised in TX and RX with sigdata containing the single frame.
        Signal_DataFrame,
        /// Raised when receiving a c-style string, pointed by sigdata.
        Signal_DataString,
        /// Raised when receiving an array of frames, pointed by sigdata.
        Signal_DataBytes,
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
    };

    UART();
    virtual ~UART();

    void init(CycleManager& cycle_manager, Logger& logger);

    void reset();

    Signal& signal();

    void set_frame_delay(cycle_count_t delay);

    void set_tx_buffer_limit(size_t limit);
    void push_tx(uint8_t frame);
    void cancel_tx_pending();
    unsigned int tx_pending() const;
    bool has_tx_collision() const;
    void clear_tx_collision();

    void set_rx_buffer_limit(size_t limit);
    void set_rx_enabled(bool enabled);
    size_t rx_available() const;
    uint8_t pop_rx();
    bool has_rx_overflow() const;
    void clear_rx_overflow();

    void set_paused(bool enabled);

    //Disable copy semantics
    UART(const UART&) = delete;
    UART& operator=(const UART&) = delete;

    //Implementation of the Signal::Hook interface to receive frames
    //from the outside
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    class RxTimer;
    class TxTimer;
    friend class RxTimer;
    friend class TxTimer;

    CycleManager* m_cycle_manager;
    Logger* m_logger;

    Signal m_signal;

    //Frame delay in clock cycles
    cycle_count_t m_delay;
    //TX FIFO buffer. The front is the TX shift register
    std::deque<uint8_t> m_tx_buffer;
    //Size limit for the TX FIFO, including the shift register
    size_t m_tx_limit;
    //Collision flag
    bool m_tx_collision;
    //Cycle timer to simulate the delay to emit a frame
    TxTimer* m_tx_timer;

    //Enable/disable flag for RX
    bool m_rx_enabled;
    //RX FIFO buffer, it has two parts, delimited by m_rx_count
    //the front part is the device actual FIFO, the back part
    //is the buffer for frames yet to be received
    std::deque<uint8_t> m_rx_buffer;
    //It is actually the no of frames in the device FIFO,
    //hence delimiting the FIFO two sub-parts
    size_t m_rx_count;
    //Size limit for the device part of the RX FIFO
    //The back part of the FIFO is not limited
    size_t m_rx_limit;
    //RX overflow flag
    bool m_rx_overflow;
    //Cycle timer to simulate the delay to receive a frame
    RxTimer* m_rx_timer;

    //Pause flag for both RX and TX
    bool m_paused;

    void add_rx_frame(uint8_t frame);
    void start_rx();

    inline bool tx_in_progress() {
        return m_tx_buffer.size() > 0;
    }

    inline bool rx_in_progress() {
        return m_rx_count < m_rx_buffer.size();
    }

    cycle_count_t rx_timer_next(cycle_count_t when);
    cycle_count_t tx_timer_next(cycle_count_t when);

};

/// Getter for the internal signal used for operation signaling.
inline Signal& UART::signal()
{
    return m_signal;
}

/// Getter for the number of frames stored in the RX buffer.
inline size_t UART::rx_available() const
{
    return m_rx_count;
}

/// Set the delay in clock ticks to emit or receive a frame. The minimum valid value is 1.
inline void UART::set_frame_delay(cycle_count_t delay)
{
    m_delay = delay ? delay : 1;
}

/// Getter for the no of frames waiting in the buffer to be emitted.
inline unsigned int UART::tx_pending() const
{
    return m_tx_buffer.size() ? (m_tx_buffer.size() - 1) : 0;
}

/// Getter for the TX collision flag
inline bool UART::has_tx_collision() const
{
    return m_tx_collision;
}

/// Clear the TX collision flag
inline void UART::clear_tx_collision()
{
    m_tx_collision = false;
}

/// Getter for the RX overflow flag
inline bool UART::has_rx_overflow() const
{
    return m_rx_overflow;
}

/// Clear the RX overflow flag
inline void UART::clear_rx_overflow()
{
    m_rx_overflow = false;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_UART_H__
