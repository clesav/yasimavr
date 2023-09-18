/*
 * sim_uart.cpp
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

#include "sim_uart.h"
#include <cstring>

YASIMAVR_USING_NAMESPACE


//=======================================================================================
/*
 * Reimplementation of cycle timers for TX and RX
 */

class UART::TxTimer : public CycleTimer {

public:

    TxTimer(UART& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override
    {
        return m_ctl.tx_timer_next(when);
    }

private:

    UART& m_ctl;

};


class UART::RxTimer : public CycleTimer {

public:

    RxTimer(UART& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override
    {
        return m_ctl.rx_timer_next(when);
    }

private:

    UART& m_ctl;

};


//=======================================================================================

UART::UART()
:m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_delay(1)
,m_tx_limit(0)
,m_tx_collision(false)
,m_rx_enabled(false)
,m_rx_count(0)
,m_rx_limit(0)
,m_rx_overflow(false)
,m_paused(false)
{
    m_rx_timer = new RxTimer(*this);
    m_tx_timer = new TxTimer(*this);
}

UART::~UART()
{
    delete m_rx_timer;
    delete m_tx_timer;
}

/**
   Initialise the interface.
   \param cycle_manager Cycle manager used for time-related operations
   \param logger Logger used for the interface
 */
void UART::init(CycleManager& cycle_manager, Logger& logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = &logger;
}

/**
   Reset the interface.
 */
void UART::reset()
{
    m_delay = 1;

    //Reset the TX part
    //Raise the signal to inform that the TX is canceled
    if (tx_in_progress())
        m_signal.raise(Signal_TX_Complete, 0);

    m_tx_buffer.clear();
    m_tx_collision = false;
    m_cycle_manager->cancel(*m_tx_timer);

    //Reset the RX part
    //Raise the signal to inform that the RX is canceled
    if (rx_in_progress())
        m_signal.raise(Signal_RX_Complete, 0);

    m_rx_enabled = false;
    m_rx_buffer.clear();
    m_rx_count = 0;
    m_rx_overflow = false;
    m_paused = false;
    m_cycle_manager->cancel(*m_rx_timer);
}


//=======================================================================================
//TX management

/**
   Set the TX buffer size, including the TX shift register.
   A zero size means unlimited.
   Stored frames are discarded to adjust if necessary.
 */
void UART::set_tx_buffer_limit(size_t limit)
{
    m_tx_limit = limit;
    while (limit > 0 && m_tx_buffer.size() > limit)
        m_tx_buffer.pop_back();
}

/**
   Push a 8-bits frame to be emitted by the interface. If no TX is already
   ongoing, it will be started immediately.
 */
void UART::push_tx(uint8_t frame)
{
    m_logger->dbg("TX push: 0x%02x ('%c')", frame, frame);

    bool tx = tx_in_progress();

    if (m_tx_limit > 0 && m_tx_buffer.size() == m_tx_limit) {
        m_tx_buffer.pop_back();
        m_tx_collision = true;
    }

    m_tx_buffer.push_back(frame);

    if (!tx) {
        m_logger->dbg("TX start: 0x%02x ('%c')", frame, frame);
        m_signal.raise(Signal_TX_Start, frame);
        m_cycle_manager->delay(*m_tx_timer, m_delay);
    }
}

/**
   Cancel all pending TX but let the current one finish, if any.
 */
void UART::cancel_tx_pending()
{
    while (m_tx_buffer.size() > 1)
        m_tx_buffer.pop_back();
}

cycle_count_t UART::tx_timer_next(cycle_count_t when)
{
    uint8_t frame = m_tx_buffer.front();
    m_tx_buffer.pop_front();

    m_logger->dbg("TX complete");

    m_signal.raise(Signal_DataFrame, frame);
    m_signal.raise(Signal_TX_Complete, 1);

    if (m_tx_buffer.size() && !m_paused) {
        uint8_t next_frame = m_tx_buffer.front();
        m_logger->dbg("TX start: 0x%02x ('%c')", next_frame, next_frame);
        m_signal.raise(Signal_TX_Start, next_frame);
        return when + m_delay;
    } else {
        return 0;
    }
}


//=======================================================================================
//RX management

/**
   Set the RX buffer size, including the RX shift register.
   A zero size means unlimited.
   Stored frames are discarded to adjust if necessary.
 */
void UART::set_rx_buffer_limit(size_t limit)
{
    m_rx_limit = limit;
    while (limit > 0 && m_rx_count > limit) {
        m_rx_buffer.pop_front();
        --m_rx_count;
    }
}

/**
   Enable/disable the reception. If disabled, the RX buffer is flushed.
 */
void UART::set_rx_enabled(bool enabled)
{
    m_rx_enabled = enabled;

    //If it's disabled, we need to cancel any RX in progress
    //and flush the front part of the FIFO
    if (!enabled) {
        if (rx_in_progress()) {
            m_signal.raise(Signal_RX_Complete, 0);
            m_cycle_manager->cancel(*m_rx_timer);
        }

        while (m_rx_count) {
            m_rx_buffer.pop_front();
            --m_rx_count;
        }

        m_rx_overflow = false;
    }
}

/**
   Pop a frame from the RX buffer.
   Use rx_available() to know if any frame is available.
   \return the popped frame, 0 if no frame is available.
 */
uint8_t UART::pop_rx()
{
    if (m_rx_count) {
        uint8_t frame = m_rx_buffer.front();
        m_rx_buffer.pop_front();
        --m_rx_count;
        m_logger->dbg("RX pop: 0x%02x ('%c')", frame, frame);
        return frame;
    } else {
        return 0;
    }
}

void UART::start_rx()
{
    //If the MCU RX buffer is full, we discard the front of the FIFO
    //and set the overrun flag
    if (m_rx_limit > 0 && m_rx_count == m_rx_limit) {
        m_rx_buffer.pop_front();
        --m_rx_count;
        m_rx_overflow = true;
    }

    //Raise a signal for the next frame to be actually received by
    //the device. It's in the front slot of the back part of the RX FIFO
    m_signal.raise(Signal_RX_Start, m_rx_buffer[m_rx_count]);
}

cycle_count_t UART::rx_timer_next(cycle_count_t when)
{
    m_logger->dbg("RX complete");

    if (m_rx_enabled && !m_paused) {
        ++m_rx_count;
        //Signal that we received a frame and kept it
        m_signal.raise(Signal_RX_Complete, 1);
    } else {
        //if disabled or paused, discard the frame just received,
        //which is in the front slot of the back part of the FIFO
        m_rx_buffer.erase(m_rx_buffer.begin() + m_rx_count);
        //Signal that we received a frame but discarded it
        m_signal.raise(Signal_RX_Complete, 0);
    }

    //Do we have further frames to receive ?
    if (m_rx_count < m_rx_buffer.size()) {
        start_rx();
        return when + m_delay;
    } else {
        return 0;
    }
}

void UART::add_rx_frame(uint8_t frame)
{
    bool timer_running = rx_in_progress();

    m_rx_buffer.push_back(frame);

    if (!timer_running) {
        start_rx();
        m_cycle_manager->delay(*m_rx_timer, m_delay);
    }
}

/*
 * Reimplementation of Signal::Hook to receive
 * data
 */
void UART::raised(const signal_data_t& sigdata, int)
{
    if (sigdata.sigid == Signal_DataFrame) {
        m_logger->dbg("RX frame received");
        add_rx_frame(sigdata.data.as_uint());
    }
    else if (sigdata.sigid == Signal_DataString) {
        m_logger->dbg("RX string received");
        const char* s = sigdata.data.as_str();
        for (size_t i = 0; i < strlen(s); ++i)
            add_rx_frame(s[i]);
    }
    else if (sigdata.sigid == Signal_DataBytes) {
        m_logger->dbg("RX bytes received");
        const uint8_t* frames = sigdata.data.as_bytes();
        size_t frame_count = sigdata.data.size();
        for (size_t i = 0; i < frame_count; i++)
            add_rx_frame(frames[i]);
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
void UART::set_paused(bool paused)
{
    //If going out of pause and there are TX frames pending, resume the transmission
    if (m_paused && !paused && m_tx_buffer.size()) {
        uint8_t next_frame = m_tx_buffer.front();
        m_logger->dbg("TX start: 0x%02x ('%c')", next_frame, next_frame);
        m_signal.raise(Signal_TX_Start, next_frame);
        m_cycle_manager->delay(*m_tx_timer, m_delay);
    }

    m_paused = paused;
}
