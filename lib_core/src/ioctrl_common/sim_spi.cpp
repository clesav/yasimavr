/*
 * sim_spi.cpp
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#include "sim_spi.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

SPIClient::SPIClient()
:m_host(nullptr)
{}

SPIClient::SPIClient(const SPIClient& other)
:SPIClient()
{
    if (other.m_host)
        other.m_host->add_client(*this);
}

SPIClient::~SPIClient()
{
    if (m_host)
        m_host->remove_client(*this);
}

SPIClient& SPIClient::operator=(const SPIClient& other)
{
    if (m_host)
        m_host->remove_client(*this);

    if (other.m_host)
        other.m_host->add_client(*this);

    return *this;
}


SPI::SPI()
:m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_delay(1)
,m_is_host(false)
,m_tfr_in_progress(false)
,m_selected(false)
,m_selected_client(nullptr)
,m_shift_reg(0)
,m_tx_limit(0)
,m_rx_limit(0)
{}

/**
   Initialise the interface.
   \param cycle_manager Cycle manager used for time-related operations
   \param logger Logger used for the interface
 */
void SPI::init(CycleManager& cycle_manager, Logger& logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = &logger;
}

/**
   Reset the interface.
 */
void SPI::reset()
{
    m_delay = 1;
    m_is_host = false;
    m_tfr_in_progress = false;
    m_selected = false;
    m_selected_client = nullptr;
    m_shift_reg = 0;

    m_tx_buffer.clear();

    m_rx_buffer.clear();

    m_cycle_manager->cancel(*this);
}

/**
   Set the interface mode.
   \param mode true=host, false=client
 */
void SPI::set_host_mode(bool mode)
{
    m_is_host = mode;
}

/**
   Add a client to the interface.
   \param client Client to add
 */
void SPI::add_client(SPIClient& client)
{
    m_clients.push_back(&client);
    client.m_host = this;
}

void SPI::remove_client(SPIClient& client)
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (*it == &client) {
            m_clients.erase(it);
            client.m_host = nullptr;
            return;
        }
    }
}

/// Set the interface as selected (client mode only)
void SPI::set_selected(bool selected)
{
    m_selected = selected;
}

/**
   Set the delay to emit or receive a frame.
   \param delay Delay in cycles. The minimum valid value is 1.
 */
void SPI::set_frame_delay(cycle_count_t delay)
{
    m_delay = delay;
}

/**
   Set the TX buffer limit and trim the buffer if necessary.
   \param limit New buffer limit. Zero means unlimited.
 */
void SPI::set_tx_buffer_limit(size_t limit)
{
    m_tx_limit = limit;
    while (limit > 0 && m_tx_buffer.size() > limit)
        m_tx_buffer.pop_back();
}

/**
   Set the RX buffer limit and trim the buffer if necessary.
   \param limit New buffer limit. Zero means unlimited.
 */
void SPI::set_rx_buffer_limit(size_t limit)
{
    m_rx_limit = limit;
    while (limit > 0 && m_rx_buffer.size() > limit)
        m_rx_buffer.pop_back();
}

/**
   Push a 8-bits frame to be emitted by the interface.
   In host mode, if no transfer is already ongoing, one will
   start immediately. Otherwise the frame is added to the TX FIFO.
   In client mode, the frame stays in the TX FIFO, waiting for
   the host to start a transfer.
 */
void SPI::push_tx(uint8_t frame)
{
    if (m_tx_limit > 0 && m_tx_buffer.size() == m_tx_limit)
        return;

    m_tx_buffer.push_back(frame);

    if (m_is_host && !m_tfr_in_progress)
        start_transfer_as_host();
}

/**
   Cancel all pending and current transfers. (host mode only)
 */
void SPI::cancel_tx()
{
    m_tx_buffer.clear();

    if (m_is_host && m_tfr_in_progress) {
        m_signal.raise(Signal_HostTfrComplete, 0);
        m_tfr_in_progress = false;
        m_rx_buffer.pop_back();
        m_cycle_manager->cancel(*this);

        if (m_selected_client) {
            m_selected_client->end_transfer(false);
            m_selected_client = nullptr;
        }
    }
}

/**
   Pop a frame from the RX buffer, return 0 if there aren't any.
 */
uint8_t SPI::pop_rx()
{
    if (m_rx_buffer.size()) {
        uint8_t frame = m_rx_buffer.front();
        m_rx_buffer.pop_front();
        m_logger->dbg("RX pop: 0x%02x ('%c')", frame, frame);
        return frame;
    } else {
        return 0;
    }
}

void SPI::start_transfer_as_host()
{
    uint8_t mosi_frame = m_tx_buffer.front();
    m_tx_buffer.pop_front();

    //Find the selected client
    m_selected_client = nullptr;
    for (SPIClient* client : m_clients) {
        if (client->selected()) {
            m_selected_client = client;
            break;
        }
    }

    //Call the selected client callback, giving it the MOSI frame
    //and it returns the MISO frame.
    //If not client is selected, the MISO line is normally pulled up therefore
    //the acquired frame is read as 0xFF.
    uint8_t miso_frame;
    if (m_selected_client)
        miso_frame = m_selected_client->start_transfer(mosi_frame);
    else
        miso_frame = 0xFF;

    m_logger->dbg("Host tfr MOSI=0x%02x, MISO=0x%02x", mosi_frame, miso_frame);

    m_signal.raise(Signal_HostTfrStart, (mosi_frame << 8) | miso_frame);

    //Add the MISO frame to the RX buffer and trim it to the limit
    m_rx_buffer.push_back(miso_frame);
    while (m_rx_limit > 0 && m_rx_buffer.size() > m_rx_limit)
        m_rx_buffer.pop_front();

    //If this is the first transfer, we need to start the timer
    if (!m_tfr_in_progress) {
        m_tfr_in_progress = true;
        m_cycle_manager->delay(*this, m_delay);
    }
}

//Timer callback indicating the end of a frame transfer.
cycle_count_t SPI::next(cycle_count_t when)
{
    //Indicate to the selected client that the transfer has completed.
    if (m_selected_client) {
        m_selected_client->end_transfer(true);
        m_selected_client = nullptr;
    }

    m_signal.raise(Signal_HostTfrComplete, 1);

    //Is there another frame to send ? if so, restart a transfer and reschedule
    //the timer
    if (m_tx_buffer.size()) {
        start_transfer_as_host();
        return when + m_delay;
    } else {
        m_tfr_in_progress = false;
        return 0;
    }
}

//=======================================================================================
/*
 * Implementation of the SPI client interface
 */

bool SPI::selected() const
{
    return m_selected;
}

uint8_t SPI::start_transfer(uint8_t mosi_frame)
{
    if (m_is_host || !m_selected || m_tfr_in_progress)
        return 0xFF;

    m_tfr_in_progress = true;

    //If we have a frame to send, pop it out of the TX buffer into the shift register
    //Note that if there is no TX frame ready, the content of the shift register
    //is the MOSI frame from the previous transfer
    uint8_t miso_frame;
    if (m_tx_buffer.size()) {
        miso_frame = m_tx_buffer.front();
        m_tx_buffer.pop_front();
    } else {
        miso_frame = m_shift_reg;
    }

    m_shift_reg = mosi_frame;

    m_signal.raise(Signal_ClientTfrStart, (mosi_frame << 8) | miso_frame);

    m_logger->dbg("Client tfr MOSI=0x%02x, MISO=0x%02x", mosi_frame, miso_frame);

    return miso_frame;
}

void SPI::end_transfer(bool ok)
{
    if (!m_tfr_in_progress) return;
    m_tfr_in_progress = false;

    if (ok) {
        //Push the MOSI frame into the RX buffer and remove old frames if the size limit
        //is reached
        m_rx_buffer.push_back(m_shift_reg);
        while (m_rx_limit > 0 && m_rx_buffer.size() > m_rx_limit)
            m_rx_buffer.pop_front();
    }

    //Inform the parent peripheral
    m_signal.raise(Signal_ClientTfrComplete, ok ? 1 : 0);
}
