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

#ifndef __YASIMAVR_IO_SPI_H__
#define __YASIMAVR_IO_SPI_H__

#include "../core/sim_types.h"
#include "../core/sim_device.h"
#include "../core/sim_signal.h"
#include <deque>
#include <vector>


//=======================================================================================
/*
 * CTLREQ definitions
*/
//Request to add a SPI client interface. data must be set to a AVR_SPI_Client object pointer
#define AVR_CTLREQ_SPI_ADD_CLIENT       1
//Request to obtain a pointer to the SPI interface as a AVR_SPI_Client object
#define AVR_CTLREQ_SPI_CLIENT           2
//Request to manually select the SPI interface when configured as a client.
//data must be set to an unsigned integer value (0 = deselected, other values = selected)
#define AVR_CTLREQ_SPI_SELECT           3

//=======================================================================================

class AVR_SPI_Client {

public:

    virtual ~AVR_SPI_Client() {};

    virtual bool selected() const = 0;
    virtual uint8_t start_transfer(uint8_t mosi_frame) = 0;
    virtual void end_transfer(bool ok) = 0;

};


class DLL_EXPORT AVR_IO_SPI : public AVR_SPI_Client, public AVR_CycleTimer {

public:

    //Signal definitions
    enum SignalId {
        Signal_HostTfrStart,
        Signal_HostTfrComplete,
        Signal_ClientTfrStart,
        Signal_ClientTfrComplete,
    };

    AVR_IO_SPI();

    //Initialise the interface. the device will be used for timer related operations
    void init(AVR_CycleManager& cycle_manager, AVR_DeviceLogger& logger);

    //Reset the interface, clear the buffers and cancel any transfer
    void reset();

    //Set the interface in host mode (mode=true) or client mode (mode=false)
    void set_host_mode(bool mode);

    //Return the internal signal used for operation signaling
    AVR_Signal& signal();

    //Set the delay in clock ticks to emit or receive a frame
    //The minimum valid value is 1
    void set_frame_delay(cycle_count_t delay);

    //Add a client to the interface
    void add_client(AVR_SPI_Client* client);

    //Set the interface as selected (client mode only)
    void set_selected(bool selected);

    //Set the TX buffer size, 0 means unlimited
    void set_tx_buffer_limit(size_t limit);

    //Push a 8-bits frame to be emitted by the interface.
    //In host mode, if no transfer is already ongoing, one will
    //start immediately.
    //In client mode, the frame is only saved until the host
    //starts a transfer
    void push_tx(uint8_t frame);

    //Cancel all transfers (host mode only)
    void cancel_tx();

    //Set the RX buffer size, 0 means unlimited
    void set_rx_buffer_limit(size_t limit);

    //Indicates if a transfer is in progress (host or client mode)
    bool tfr_in_progress() const;

    //Count of frames in the RX buffer
    size_t rx_available() const;

    //Pop a frame from the RX buffer, return 0 if there aren't any
    uint8_t pop_rx();

    //Reimplementation of AVR_CycleTimer interface
    virtual cycle_count_t next(cycle_count_t when) override;
    //Reimplementation of AVR_SPI_Client interface
    virtual bool selected() const override;
    virtual uint8_t start_transfer(uint8_t mosi_frame) override;
    virtual void end_transfer(bool ok) override;

private:

    AVR_CycleManager* m_cycle_manager;
    AVR_DeviceLogger* m_logger;
    cycle_count_t m_delay;
    bool m_is_host;
    bool m_tfr_in_progress;
    bool m_selected;
    std::vector<AVR_SPI_Client*> m_clients;
    AVR_SPI_Client* m_selected_client;

    uint8_t m_shift_reg;

    std::deque<uint8_t> m_tx_buffer;
    size_t m_tx_limit;

    std::deque<uint8_t> m_rx_buffer;
    size_t m_rx_limit;

    AVR_Signal m_signal;

    void start_transfer_as_host();

};

inline AVR_Signal& AVR_IO_SPI::signal()
{
    return m_signal;
}

inline size_t AVR_IO_SPI::rx_available() const
{
    size_t n = m_rx_buffer.size();
    if (m_tfr_in_progress) --n;
    return n;
}

inline bool AVR_IO_SPI::tfr_in_progress() const
{
    return m_tfr_in_progress;
}


#endif //__YASIMAVR_IO_SPI_H__
