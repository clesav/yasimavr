/*
 * sim_spi.cpp
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

#include "sim_spi.h"


//=======================================================================================

AVR_IO_SPI::AVR_IO_SPI()
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

void AVR_IO_SPI::init(AVR_CycleManager& cycle_manager, AVR_DeviceLogger& logger)
{
	m_cycle_manager = &cycle_manager;
	m_logger = &logger;
}

void AVR_IO_SPI::reset()
{
	m_delay = 1;
	m_is_host = false;
	m_tfr_in_progress = false;
	m_selected = false;
	m_selected_client = nullptr;
	m_shift_reg = 0;

	m_tx_buffer.clear();

	m_rx_buffer.clear();

	m_cycle_manager->remove_cycle_timer(this);
}

void AVR_IO_SPI::set_host_mode(bool mode)
{
	m_is_host = mode;
}

void AVR_IO_SPI::add_client(AVR_SPI_Client* client)
{
	m_clients.push_back(client);
}

void AVR_IO_SPI::set_selected(bool selected)
{
	m_selected = selected;
}

void AVR_IO_SPI::set_frame_delay(cycle_count_t delay)
{
	m_delay = delay;
}

/*
 * Set the TX buffer limit and trim the buffer if necessary
 */
void AVR_IO_SPI::set_tx_buffer_limit(unsigned int limit)
{
	m_tx_limit = limit;
	while (limit > 0 && m_tx_buffer.size() > limit)
		m_tx_buffer.pop_back();
}

/*
 * Set the RX buffer limit and trim the buffer if necessary
 */
void AVR_IO_SPI::set_rx_buffer_limit(unsigned int limit)
{
	m_rx_limit = limit;
	while (limit > 0 && m_rx_buffer.size() > limit)
		m_rx_buffer.pop_back();
}

/*
 * Push a 8-bits frame to be transmitted
 * Start the transfer if it's not already in progress
 */
void AVR_IO_SPI::push_tx(uint8_t frame)
{
	if (m_tx_limit > 0 && m_tx_buffer.size() == m_tx_limit)
		return;

	m_tx_buffer.push_back(frame);

	if (m_is_host && !m_tfr_in_progress)
		start_transfer();
}

/*
 * Cancel all pending and current transfers
 */
void AVR_IO_SPI::cancel_tx()
{
	m_tx_buffer.clear();

	if (m_is_host && m_tfr_in_progress) {
		m_signal.raise_u(Signal_HostTfrComplete, 0, 0);
		m_tfr_in_progress = false;
		m_cycle_manager->remove_cycle_timer(this);

		if (m_selected_client) {
			m_selected_client->end_transfer(false);
			m_selected_client = nullptr;
		}
	}
}

uint8_t AVR_IO_SPI::pop_rx()
{
	if (m_rx_buffer.size()) {
		uint8_t frame = m_rx_buffer.front();
		m_rx_buffer.pop_front();
		DEBUG_LOG(*m_logger, "SPI RX pop: 0x%02x ('%c')", frame, frame);
		return frame;
	} else {
		return 0;
	}
}

void AVR_IO_SPI::start_transfer()
{
	uint8_t mosi_frame = m_tx_buffer.front();
	m_tx_buffer.pop_front();

	//Find the selected client
	m_selected_client = nullptr;
	for (AVR_SPI_Client* client : m_clients) {
		if (client->selected()) {
			m_selected_client = client;
			break;
		}
	}

	//call it, giving it the MOSI frame and it returns the MISO frame
	uint8_t miso_frame;
	if (m_selected_client)
		miso_frame = m_selected_client->start_transfer(mosi_frame);
	else
		miso_frame = 0xFF;

	DEBUG_LOG(*m_logger, "SPI host tfr MOSI=0x%02x, MISO=0x%02x", mosi_frame, miso_frame);

	m_signal.raise_u(Signal_HostTfrStart, 0, (mosi_frame << 8) | miso_frame);

	//Add the MISO frame to the RX buffer and trim it to the limit
	m_rx_buffer.push_back(miso_frame);
	while (m_rx_limit > 0 && m_rx_buffer.size() > m_rx_limit)
		m_rx_buffer.pop_front();

	//If this is the first transfer, we need to start the timer
	if (!m_tfr_in_progress) {
		m_tfr_in_progress = true;
		m_cycle_manager->add_cycle_timer(this, m_cycle_manager->cycle() + m_delay);
	}
}

cycle_count_t AVR_IO_SPI::next(cycle_count_t when)
{
	m_selected_client->end_transfer(true);
	m_selected_client = nullptr;

	m_signal.raise_u(Signal_HostTfrComplete, 0, 1);

	//Is there another transfer to do ?
	if (m_tx_buffer.size()) {
		start_transfer();
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

bool AVR_IO_SPI::selected() const
{
	return m_selected;
}

uint8_t AVR_IO_SPI::start_transfer(uint8_t mosi_frame)
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

	m_signal.raise_u(Signal_ClientTfrStart, 0, (mosi_frame << 8) | miso_frame);

	DEBUG_LOG(*m_logger, "SPI client tfr MOSI=0x%02x, MISO=0x%02x", mosi_frame, miso_frame);

	return miso_frame;
}

void AVR_IO_SPI::end_transfer(bool ok)
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
	m_signal.raise_u(Signal_ClientTfrComplete, 0, ok ? 1 : 0);
}
