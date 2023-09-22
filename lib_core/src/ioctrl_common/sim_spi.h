/*
 * sim_spi.h
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

#ifndef __YASIMAVR_SPI_H__
#define __YASIMAVR_SPI_H__

#include "../core/sim_types.h"
#include "../core/sim_device.h"
#include "../core/sim_signal.h"
#include <deque>
#include <vector>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_spi Serial Peripheral Interface framework
   @{
 */

/**
   \name Controller requests definition for SPI
   @{
 */

/**
   Request to register a SPI client to a SPI interface
    - data must be a pointer to a SPIClient object
 */
#define AVR_CTLREQ_SPI_ADD_CLIENT       1

/**
   Request to obtain a pointer to the SPI interface as a SPI client
    - data is returned as a pointer to a SPIClient object
 */
#define AVR_CTLREQ_SPI_CLIENT           2

/**
   Request to select/deselect the SPI interface when used as a client
    - data must be an integer : select if > 0, deselect if == 0
 */
#define AVR_CTLREQ_SPI_SELECT           3

/// @}
/// @}


//=======================================================================================

class SPI;

/**
   \ingroup api_spi
   \brief Abstract interface for a SPI Client.
 */
class AVR_CORE_PUBLIC_API SPIClient {

public:

    SPIClient();
    SPIClient(const SPIClient& other);
    virtual ~SPIClient();

    /// Used by the SPI host to interrogate the selection state of the client.
    virtual bool selected() const = 0;

    /**
       Called by the SPI host to start a transfer of one frame.
       \param mosi_frame MOSI frame emitted by the host
       \return the MISO frame simultaneously emitted by the client.
     */
    virtual uint8_t start_transfer(uint8_t mosi_frame) = 0;

    /**
       Called by the host at the end of a transfer.
       \param ok indicates the success of the transfer, or false if it was aborted.
     */
    virtual void end_transfer(bool ok) = 0;

    SPIClient& operator=(const SPIClient& other);

private:

    friend class SPI;

    SPI* m_host;

};


/**
   \ingroup api_spi
   \brief Generic model defining an serial peripheral interface a.k.a. SPI.

    The interface can act in either host or client mode.

    The class is composed of two FIFOs, one for TX, the other for RX.
    The transfer of a frame starts immediately after pushing it in the TX FIFO.
 */
class AVR_CORE_PUBLIC_API SPI : public SPIClient, public CycleTimer {

public:

    /// Signal Ids raised by this object.
    enum SignalId {
        Signal_HostTfrStart,
        Signal_HostTfrComplete,
        Signal_ClientTfrStart,
        Signal_ClientTfrComplete,
    };

    SPI();

    void init(CycleManager& cycle_manager, Logger& logger);

    void reset();

    void set_host_mode(bool mode);
    bool is_host_mode() const;

    Signal& signal();

    void set_frame_delay(cycle_count_t delay);

    void add_client(SPIClient& client);

    void remove_client(SPIClient& client);

    void set_selected(bool selected);

    void set_tx_buffer_limit(size_t limit);

    void push_tx(uint8_t frame);

    void cancel_tx();

    void set_rx_buffer_limit(size_t limit);

    //Indicates if a transfer is in progress (host or client mode)
    bool tfr_in_progress() const;


    size_t rx_available() const;

    uint8_t pop_rx();

    //Reimplementation of CycleTimer interface
    virtual cycle_count_t next(cycle_count_t when) override;

    //Reimplementation of SPIClient interface
    virtual bool selected() const override;
    virtual uint8_t start_transfer(uint8_t mosi_frame) override;
    virtual void end_transfer(bool ok) override;

private:

    CycleManager* m_cycle_manager;
    Logger* m_logger;
    cycle_count_t m_delay;
    bool m_is_host;
    bool m_tfr_in_progress;
    bool m_selected;
    std::vector<SPIClient*> m_clients;
    SPIClient* m_selected_client;

    uint8_t m_shift_reg;

    std::deque<uint8_t> m_tx_buffer;
    size_t m_tx_limit;

    std::deque<uint8_t> m_rx_buffer;
    size_t m_rx_limit;

    Signal m_signal;

    void start_transfer_as_host();

};

/// Getter for the signal raised during transfers
inline Signal& SPI::signal()
{
    return m_signal;
}

inline bool SPI::is_host_mode() const
{
    return m_is_host;
}

/// Getter for the count of frames in the RX buffer
inline size_t SPI::rx_available() const
{
    size_t n = m_rx_buffer.size();
    if (m_tfr_in_progress) --n;
    return n;
}

/// Getter indicating if a transfer is in progress
inline bool SPI::tfr_in_progress() const
{
    return m_tfr_in_progress;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_SPI_H__
