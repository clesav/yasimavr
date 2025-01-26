/*
 * twi.sip
 *
 *  Copyright 2021-2025 Clement Savergne <csavergne@yahoo.com>

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

#include "ioctrl_common/sim_twi.h"

const ctlreq_id_t CTLREQ_TWI_BUS_ERROR = AVR_CTLREQ_TWI_BUS_ERROR;

%End


const ctlreq_id_t CTLREQ_TWI_BUS_ERROR;


//=======================================================================================

namespace TWI {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End


//=======================================================================================

enum Line /BaseType=IntEnum/ {
    Line_Clock   /PyName=Clock/,
    Line_Data    /PyName=Data/,
};


enum BusState /BaseType=IntEnum/ {
    Bus_Idle     /PyName=Idle/,
    Bus_Busy     /PyName=Busy/,
    Bus_Owned    /PyName=Owned/,
};


enum SignalId /BaseType=IntEnum/ {
    Signal_StateChanged      /PyName=StateChanged/,
    Signal_BusStateChanged   /PyName=BusStateChanged/,
    Signal_Start             /PyName=Start/,
    Signal_AddressStandby    /PyName=AddressStandby/,
    Signal_AddressSent       /PyName=AddressSent/,
    Signal_AddressReceived   /PyName=AddressReceived/,
    Signal_DataStandby       /PyName=DataStandby/,
    Signal_DataSent          /PyName=DataSent/,
    Signal_DataAckReceived   /PyName=DataAckReceived/,
    Signal_DataReceived      /PyName=DataReceived/,
    Signal_DataAckSent       /PyName=DataAckSent/,
    Signal_ArbitrationLost   /PyName=ArbitrationLost/,
    Signal_BusCollision      /PyName=BusCollision/,
    Signal_Stop              /PyName=Stop/,
};


class EndPoint {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    EndPoint();

    void line_state_changed(Line, bool);

    bool get_clock_drive() const;
    bool get_data_drive() const;

protected:

    virtual void clock_level_changed(bool) = 0;
    virtual void data_level_changed(bool) = 0;
    virtual void set_line_state(Line, bool) = 0;

    void set_clock_drive(bool);
    bool get_clock_level() const;
    void set_data_drive(bool);
    bool get_data_level() const;

};


//=======================================================================================

class Client : public EndPoint, public CycleTimer {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    enum State /BaseType=IntEnum/ {
        State_Disabled      /PyName=Disabled/,
        State_Idle          /PyName=Idle/,
        State_Start         /PyName=Start/,
        State_AddressRx     /PyName=AddressRx/,
        State_AddressRAck   /PyName=AddressRAck/,
        State_AddressWAck   /PyName=AddressWAck/,
        State_DataTx        /PyName=DataTx/,
        State_DataTxAck     /PyName=DataTxAck/,
        State_DataRx        /PyName=DataRx/,
        State_DataRxAck     /PyName=DataRxAck/,
    };

    Client();

    void init(CycleManager&);
    State state() const;
    void set_enabled(bool);
    bool enabled() const;
    void reset();
    bool active() const;
    bool clock_hold() const;
    unsigned char rw() const /PyInt/;
    bool ack() const;
    void set_ack(bool);
    void start_data_tx(uint8_t);
    void start_data_rx();
    Signal& signal();

    virtual cycle_count_t next(cycle_count_t);

protected:

    virtual void clock_level_changed(bool);
    virtual void data_level_changed(bool);

};


//=======================================================================================

class Host : public EndPoint, public CycleTimer {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    enum State /BaseType=IntEnum/ {
        State_Disabled      /PyName=Disabled/,
        State_Idle          /PyName=Idle/,
        State_Start         /PyName=Start/,
        State_AddressTx     /PyName=AddressTx/,
        State_AddressAck    /PyName=AddressAck/,
        State_DataTx        /PyName=DataTx/,
        State_DataTxAck     /PyName=DataTxAck/,
        State_DataRx        /PyName=DataRx/,
        State_DataRxAck     /PyName=DataRxAck/,
        State_Stop          /PyName=Stop/,
        State_BusBusy       /PyName=BusBusy/,
        State_ArbLost       /PyName=ArbLost/,
    };

    Host();

    void init(CycleManager&);
    State state() const;
    void set_enabled(bool);
    bool enabled() const;
    void reset();
    void set_bit_delay(cycle_count_t);
    bool bus_busy() const;
    bool active() const;
    bool clock_hold() const;
    bool clock_stretched() const;
    unsigned char rw() const /PyInt/;
    bool ack() const;
    bool start_transfer();
    bool set_address(uint8_t);
    bool set_ack(bool);
    bool start_data_tx(uint8_t);
    void start_data_rx();
    bool stop_transfer();
    Signal& signal();

    virtual cycle_count_t next(cycle_count_t);

protected:

    virtual void clock_level_changed(bool);
    virtual void data_level_changed(bool);

};

}; //namespace TWI
