/*
 * spi.sip
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

%ModuleCode

#include "ioctrl_common/sim_spi.h"

const ctlreq_id_t CTLREQ_SPI_ADD_CLIENT = AVR_CTLREQ_SPI_ADD_CLIENT;
const ctlreq_id_t CTLREQ_SPI_CLIENT = AVR_CTLREQ_SPI_CLIENT;
const ctlreq_id_t CTLREQ_SPI_SELECT = AVR_CTLREQ_SPI_SELECT;

%End

const ctlreq_id_t CTLREQ_SPI_ADD_CLIENT;
const ctlreq_id_t CTLREQ_SPI_CLIENT;
const ctlreq_id_t CTLREQ_SPI_SELECT;


//=======================================================================================

class SPIClient {
%TypeHeaderCode
#include "ioctrl_common/sim_spi.h"
%End

public:

    SPIClient();
    SPIClient(const SPIClient&);

    virtual bool selected() const = 0;
    virtual uint8_t start_transfer(uint8_t mosi_frame) = 0;
    virtual void end_transfer(bool ok) = 0;

};


class SPI : public SPIClient, public CycleTimer /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_spi.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_HostTfrStart         /PyName=HostTfrStart/,
        Signal_HostTfrComplete      /PyName=HostTfrComplete/,
        Signal_ClientTfrStart       /PyName=ClientTfrStart/,
        Signal_ClientTfrComplete    /PyName=ClientTfrComplete/,
    };

    SPI();

    void init(CycleManager&, Logger&);
    void reset();
    void set_host_mode(bool);
    bool is_host_mode() const;
    Signal& signal();
    void set_frame_delay(cycle_count_t);
    void add_client(SPIClient&);
    void remove_client(SPIClient&);
    void set_selected(bool);
    void set_tx_buffer_limit(size_t);
    void push_tx(uint8_t);
    void cancel_tx();
    void set_rx_buffer_limit(size_t);
    bool tfr_in_progress() const;
    size_t rx_available() const;
    uint8_t pop_rx();
    virtual cycle_count_t next(cycle_count_t);
    virtual bool selected() const;
    virtual uint8_t start_transfer(uint8_t);
    virtual void end_transfer(bool);

};
