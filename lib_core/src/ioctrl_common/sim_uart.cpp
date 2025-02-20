/*
 * sim_uart.cpp
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

#include "sim_uart.h"

YASIMAVR_USING_NAMESPACE

using namespace UART;


//=======================================================================================

USART::USART()
:m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_delay(2)
,m_clk_mode(Clock_Async)
,m_clk_timer(*this, &USART::clk_timer_next)
,m_databits(8)
,m_stopbits(1)
,m_parity(Parity_No)
,m_tx_shifter(0x0000)
,m_tx_shift_counter(0)
,m_tx_limit(0)
,m_tx_dir_enabled(false)
,m_tx_timer(*this, &USART::tx_timer_next)
,m_rx_enabled(false)
,m_rx_shifter(0x0000)
,m_rx_shift_counter(0)
,m_rx_limit(0)
,m_rx_timer(*this, &USART::rx_timer_next)
,m_paused(false)
{}

/**
   Initialise the interface.
   \param cycle_manager Cycle manager used for time-related operations
   \param logger Logger used for the interface
 */
void USART::init(CycleManager& cycle_manager, Logger* logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = logger;
}

/**
   Reset the interface.
 */
void USART::reset()
{
    m_delay = 2;
    m_clk_mode = Clock_Async;
    m_cycle_manager->cancel(m_clk_timer);
    set_line_state(Line_XCK, false);

    m_databits = 8;
    m_stopbits = 1;
    m_parity = Parity_No;

    if (m_tx_shift_counter)
        m_signal.raise(Signal_TX_Complete, 0);

    m_tx_shifter = 0x0000;
    m_tx_buffer.clear();
    m_tx_shift_counter = 0;
    m_cycle_manager->cancel(m_tx_timer);

    set_line_state(Line_TXD, true);
    set_line_state(Line_DIR, false);

    //Reset the RX part
    //Raise the signal to inform that the RX is cancelled
    if (m_rx_shift_counter || m_rx_pending.size())
        m_signal.raise(Signal_RX_Complete, 0);

    m_rx_enabled = false;
    m_rx_shifter = 0x0000;
    m_rx_shift_counter = 0;
    m_rx_buffer.clear();
    m_rx_pending.clear();
    m_cycle_manager->cancel(m_rx_timer);

    m_paused = false;
}


//=======================================================================================
//Clock generation management

/**
   Set the clock mode.
 */
void USART::set_clock_mode(ClockMode mode)
{
    m_clk_mode = mode;
    if (mode == Clock_Emitter) {
        m_cycle_manager->delay(m_clk_timer, m_delay / 2);
    } else {
        m_cycle_manager->cancel(m_clk_timer);
        set_line_state(Line_XCK, false);
    }
}

/**
   Set the bit duration delay in clock ticks to emit or receive a frame.
   The minimum valid value is 2.
 */
void USART::set_bit_delay(cycle_count_t delay)
{
    if (delay < 2) delay = 2;
    m_delay = delay;
}

/*
   Private callback for clock generation, scheduled every half-baud period:
      - toggle the clock line state
      - process the changes for the TX and RX stages
 */
cycle_count_t USART::clk_timer_next(cycle_count_t when)
{
    bool new_state = !get_line_state(Line_XCK);
    set_line_state(Line_XCK, new_state);
    process_clock_change(new_state);
    return when + m_delay / 2;
}

/*
   Private method to process the changes of the clock line for the TX and RX stages
 */
void USART::process_clock_change(bool new_state)
{
    if (new_state) {
        //On a positive edge, shift the TX register out
        if (m_tx_shift_counter)
            shift_tx();
    } else {
        //On a negative edge:
        // - If a reception is already in progress, sample and shift the RX register
        // - else, if RX is enabled and the RXD line is low, start a reception
        if (m_rx_shift_counter)
            shift_rx();
        else if (m_rx_enabled && !get_line_state(Line_RXD))
            start_bitwise_rx();
    }
}


//=======================================================================================
//Frame format management

/**
   Set the number of stop bits. Valid values are 1 and 2.
 */
void USART::set_stopbits(unsigned short count)
{
    if (count == 1 || count == 2)
        m_stopbits = count;
}

/**
   Set the number of data bits. Valid values are 5 to 9.
 */
void USART::set_databits(unsigned short count)
{
    if (count >= 5 && count <= 9)
        m_databits = count;
}

/**
   Set the parity mode.
 */
void USART::set_parity(Parity parity)
{
    m_parity = parity;
}


unsigned short USART::framesize() const
{
    return 1 + m_databits + (m_parity == Parity_No ? 0 : 1) + m_stopbits;
}


static uint16_t parity_odd(uint16_t bits, unsigned short count)
{
    uint16_t p = 0;
    for (unsigned short i = 0; i < count; ++i) {
        p ^= bits & 0x01;
        bits >>= 1;
    }
    return p;
}

/**
   Builds a USART frame.
   \param data data bits
   \return the frame bits, with the start bit at bit 0
 */
uint16_t USART::build_frame(uint16_t data) const
{
    //The construction of the frame is done in reverse. First, the stop bits.
    uint16_t frame = (m_stopbits == 2) ? 0x0003 : 0x0001;

    //Add the parity bit if enabled
    if (m_parity != Parity_No) {
        uint16_t p = parity_odd(data, m_databits);
        if (m_parity == Parity_Even) p ^= 0x01;
        frame = (frame << 1) | p;
    }

    //Ensure the data is limited to m_databits
    data &= ((uint16_t) 1 << m_databits) - 1;
    frame = (frame << m_databits) | data;

    //Add the start bit
    frame <<= 1;

    return frame;
}

/**
   Parses a full frame : extract the data bits and check for a parity or frame errors.
   \return the data bits, with the 15th bit is set for the parity error, and
   the 14th bit is set for a frame error (a stop bit not equal to 1)
 */
uint16_t USART::parse_frame(uint16_t frame) const
{
    //Extract the data bits
    frame >>= 1;
    uint16_t parsed_data = frame & (((uint16_t) 1 << m_databits) - 1);
    frame >>= m_databits;

    //Extract and check the parity bit
    bool perr = false;
    if (m_parity != Parity_No) {
        uint16_t p_from_frame = frame & 0x0001;
        uint16_t p_from_data = parity_odd(parsed_data, m_databits);
        if (m_parity == Parity_Even) p_from_data ^= 1;
        perr = (p_from_frame != p_from_data);
        frame >>= 1;
    }

    //Check the stop bits : they should all be 1
    bool ferr = !(frame & 0x0001) || ((m_stopbits == 2) && !(frame & 0x0002));

    //Store the parity & frame error flags in the 14th and 15th bits
    if (perr) parsed_data |= 0x8000;
    if (ferr) parsed_data |= 0x4000;

    return parsed_data;
}


//=======================================================================================
//TX management

/**
   Set the TX buffer size, including the TX shift register.
   A zero size means unlimited.
   Stored frames are discarded to adjust if necessary.
 */
void USART::set_tx_buffer_limit(size_t limit)
{
    m_tx_limit = limit;
    while (limit > 0 && m_tx_buffer.size() > limit)
        m_tx_buffer.pop_back();
}

/**
   Push a data frame to be emitted by the interface. If no TX is already
   ongoing, it will be started immediately.
 */
void USART::push_tx(uint16_t frame)
{
    if (m_logger) m_logger->dbg("TX push: 0x%03x", frame);

    if (m_tx_limit > 0 && m_tx_buffer.size() == m_tx_limit) {
        m_tx_buffer.pop_back();
        m_signal.raise(Signal_TX_Collision);
    }

    m_tx_buffer.push_back(frame);

    if (!m_tx_shift_counter && !m_paused)
        start_bitwise_tx();
}


void USART::start_bitwise_tx()
{
    fill_tx_shifter();

    if (m_tx_dir_enabled)
        set_line_state(Line_DIR, true);

    if (m_clk_mode == Clock_Async) {
        shift_tx();
        m_cycle_manager->delay(m_tx_timer, m_delay);
    }
}

/**
   Enable/disable the controls of the DIR line during transmission.
 */
void USART::set_tx_dir_enabled(bool enabled)
{
    m_tx_dir_enabled = enabled;
}

/**
   Returns whether a transmission is in progress
 */
bool USART::tx_in_progress() const
{
    return !!m_tx_shift_counter;
}


void USART::fill_tx_shifter()
{
    uint16_t data = m_tx_buffer.front();
    m_tx_shifter = build_frame(data);
    m_tx_shift_counter = framesize() + 1;

    //if the DIR line is used, add an extra baud cycle at the start as guard
    if (m_tx_dir_enabled) {
        m_tx_shifter = (m_tx_shifter << 1) | 1;
        ++m_tx_shift_counter;
    }

    if (m_logger) m_logger->dbg("TX start: 0x%03x", data);
    m_signal.raise(Signal_TX_Start, data);
    m_signal.raise(Signal_TX_Frame, m_tx_shifter);
}


void USART::shift_tx()
{
    if (m_tx_shift_counter == 1) {
        //The current frame transmission is complete, raise the signals
    	if (m_logger) m_logger->dbg("TX complete");
        m_signal.raise(Signal_TX_Data, m_tx_buffer.front());
        m_signal.raise(Signal_TX_Complete, 1);

        //Move to the next frame to send, if any.
        //If not, release the DIR line (if used)
        m_tx_buffer.pop_front();
        if (m_tx_buffer.size() && !m_paused)
            fill_tx_shifter();
        else if (m_tx_dir_enabled)
            set_line_state(Line_DIR, false);
    }

    if (m_tx_shift_counter > 1) {
        set_line_state(Line_TXD, m_tx_shifter & 1);
        m_tx_shifter >>= 1;
    }

    --m_tx_shift_counter;
}

/**
   Cancel all pending TX but let the current one finish, if any.
 */
void USART::cancel_tx_pending()
{
    while (m_tx_buffer.size() > 1)
        m_tx_buffer.pop_back();
}


cycle_count_t USART::tx_timer_next(cycle_count_t when)
{
    shift_tx();
    return m_tx_shift_counter ? (when + m_delay) : 0;
}


//=======================================================================================
//RX management

/**
   Set the RX buffer size, including the RX shift register.
   A zero size means unlimited.
   Stored frames are discarded to adjust if necessary.
 */
void USART::set_rx_buffer_limit(size_t limit)
{
    m_rx_limit = limit;
    while (limit > 0 && m_rx_buffer.size() > limit)
        m_rx_buffer.pop_front();
}

/**
   Enable/disable the reception. If disabled, the RX buffer is flushed.
 */
void USART::set_rx_enabled(bool enabled)
{
    m_rx_enabled = enabled;

    //If it's disabled, we need to cancel any RX in progress
    //and flush the front part of the FIFO
    if (!enabled) {
        if (m_rx_shift_counter) {
            m_signal.raise(Signal_RX_Complete, 0);
            m_cycle_manager->cancel(m_rx_timer);
            m_rx_shift_counter = 0;
        }

        m_rx_buffer.clear();
    }
}

/**
   Pop a frame from the RX buffer.
   Use rx_available() to know if any frame is available.
 */
void USART::pop_rx()
{
    if (m_rx_buffer.size())
        m_rx_buffer.pop_front();
}

/**
   Read the front frame from the RX buffer.
   Use rx_available() to know if any frame is available.
   \return the frame, 0 if no frame is available.
 */
uint16_t USART::read_rx() const
{
    return m_rx_buffer.size() ? (m_rx_buffer.front() & 0x0FFF) : 0;
}


bool USART::has_frame_error() const
{
    return m_rx_buffer.size() && (m_rx_buffer.front() & 0x4000);
}


bool USART::has_parity_error() const
{
    return m_rx_buffer.size() && (m_rx_buffer.front() & 0x8000);
}


bool USART::has_rx_overrun() const
{
    return m_rx_buffer.size() && (m_rx_buffer.front() & 0x2000);
}


bool USART::rx_in_progress() const
{
    return m_rx_shift_counter || m_rx_pending.size();
}


void USART::start_bitwise_rx()
{
    //Check if we have frames pending reception in the buffer and discard those.
    if (m_rx_pending.size()) {
        m_cycle_manager->cancel(m_rx_timer);
        m_rx_pending.clear();
    }

    //Initialise the shift register and counter
    m_rx_shifter = 0x0000;
    m_rx_shift_counter = framesize();

    //Raise a signal to indicate the start of a reception
    m_signal.raise(Signal_RX_Start);

    //If the MCU RX buffer is full
    if (m_rx_limit > 0 && m_rx_buffer.size() == m_rx_limit) {
        //we discard the back of the FIFO, i.e. the last frame received is lost
        m_rx_buffer.pop_back();
        //Set the flag in the front slot
        m_rx_buffer[0] |= 0x2000;
        //raise the overflow signal
        m_signal.raise(Signal_RX_Overflow);
    }

    //If asynchronous, we need our own clock.
    //The first delay is a 1.5 baud period in order to sample in the middle of the next bit.
    if (m_clk_mode == Clock_Async)
        m_cycle_manager->delay(m_rx_timer, m_delay + m_delay / 2);
}

/**
   Push frames into a buffer for bytewise reception and start receiving them.
   The frames are only accepted if no bitwise reception is progress and the clock is asynchronous.
 */
void USART::push_rx_frame(uint16_t frame)
{
    if (m_rx_shift_counter || m_clk_mode != Clock_Async) return;

    m_rx_pending.push_back(frame);

    if (!m_rx_timer.scheduled())
        m_cycle_manager->delay(m_rx_timer, m_delay * framesize());
}


void USART::shift_rx()
{
    if (m_rx_shift_counter > 1) {
        m_rx_shifter >>= 1;
        if (get_line_state(Line_RXD))
            m_rx_shifter |= 0x8000;
    }
    --m_rx_shift_counter;

    if (!m_rx_shift_counter) {
        m_rx_shifter >>= 16 - framesize();
        uint16_t data = parse_frame(m_rx_shifter);
        m_rx_buffer.push_back(data);
        if (m_logger) m_logger->dbg("Frame RX (bitwise) Complete: 0x%04x", data);
        m_signal.raise(Signal_RX_Complete, 1);
    }
}


cycle_count_t USART::rx_timer_next(cycle_count_t when)
{
    cycle_count_t next_when;

    if (m_rx_shift_counter) {

        //Bitwise reception
        shift_rx();
        if (!m_rx_shift_counter)
            next_when = 0;
        else if (m_rx_shift_counter == 1)
            next_when = when + m_delay / 2;
        else
            next_when = when + m_delay;

    } else {

        //Bytewise reception
        if (m_rx_enabled && !m_paused) {
            //Transfer from the pending queue to the RX FIFO
            uint16_t data = m_rx_pending.front();
            m_rx_pending.pop_front();
            m_rx_buffer.push_back(data);
            if (m_logger) m_logger->dbg("Frame RX (bytewise) Complete: 0x%03x", data);
            m_signal.raise(Signal_RX_Complete, 1);
        } else {
            //If disabled or paused, discard the frame
            m_rx_pending.pop_front();
            //Signal that we received a frame but discarded it
            m_signal.raise(Signal_RX_Complete, 0);
        }
        next_when = m_rx_pending.size() ? (when + m_delay * framesize()) : 0;

    }

    return next_when;
}


//=======================================================================================
//Line change management

void USART::line_state_changed(Line line, bool new_state)
{
    if (line == Line_RXD) {
        //If RX is enabled, a bitwise reception is not in progress and the clock is asynchronous,
        //a negative edge starts the reception of a frame
        if (m_rx_enabled && !m_rx_shift_counter && m_clk_mode == Clock_Async && !new_state)
            start_bitwise_rx();
    }
    else if (line == Line_XCK && m_clk_mode == Clock_Receiver) {
        process_clock_change(new_state);
    }
}


//=======================================================================================
//Pause management

/**
   Enable/disable the pause mode.

   If pause is enabled, any ongoing communication will complete as normal, and
   further TX frames won't be emitted (but remain in the FIFO). Frames already
   in the RX FIFO are kept but further received frames will be ignored.
 */
void USART::set_paused(bool paused)
{
    //If going out of pause and there are TX frames pending, resume the transmission
    if (m_paused && !paused && m_tx_buffer.size())
        start_bitwise_tx();

    m_paused = paused;
}
