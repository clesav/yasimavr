/*
 * twi.sip
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

#include "ioctrl_common/sim_twi.h"

const ctlreq_id_t CTLREQ_TWI_ENDPOINT = AVR_CTLREQ_TWI_ENDPOINT;

%End


//=======================================================================================

const ctlreq_id_t CTLREQ_TWI_ENDPOINT;

class TWIPacket {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    enum Cmd /BaseType=IntEnum/ {
        Cmd_Invalid         /PyName=Invalid/,
        Cmd_Address         /PyName=Address/,
        Cmd_AddrAck         /PyName=AddrAck/,
        Cmd_DataRequest     /PyName=DataRequest/,
        Cmd_Data            /PyName=Data/,
        Cmd_DataAck         /PyName=DataAck/,
    };

    enum /BaseType=IntEnum/ {
        Nack,
        Ack,
        Write,
        Read,
    };

    uint8_t cmd;
    uint8_t addr;
    uint8_t rw;
    uint8_t data;
    uint8_t ack;
    uint8_t hold;

    TWIPacket();

};


class TWIEndPoint /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    TWIEndPoint();

    TWIBus* bus() const;

protected:

    bool acquire_bus();
    void release_bus();
    void send_packet(TWIPacket&);
    void end_packet(TWIPacket&);

    virtual void packet(TWIPacket&) = 0;
    virtual void packet_ended(TWIPacket&) = 0;
    virtual void bus_acquired() = 0;
    virtual void bus_released() = 0;

};


class TWIBus /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_Start        /PyName=Start/,
        Signal_Address      /PyName=Address/,
        Signal_Data         /PyName=Data/,
        Signal_Packet_End   /PyName=Packet_End/,
        Signal_Ack          /PyName=Ack/,
        Signal_Stop         /PyName=Stop/,
    };

    TWIBus();

    Signal& signal();

    void add_endpoint(TWIEndPoint&);
    void remove_endpoint(TWIEndPoint&);

};

class TWI : public TWIEndPoint /NoDefaultCtors/ {
%TypeHeaderCode
#include "ioctrl_common/sim_twi.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_StateChange      /PyName=StateChange/,
        Signal_BusStateChange   /PyName=BusStateChange/,
        Signal_Address          /PyName=Address/,
        Signal_AddrAck          /PyName=AddrAck/,
        Signal_TxComplete       /PyName=TxComplete/,
        Signal_RxComplete       /PyName=RxComplete/,
    };

    enum Component /BaseType=IntEnum/ {
        Cpt_Any                 /PyName=Any/,
        Cpt_Master              /PyName=Master/,
        Cpt_Slave               /PyName=Slave/,
    };

    enum BusState /BaseType=IntEnum/ {
        Bus_Idle,
        Bus_Busy,
        Bus_Owned,
    };

    enum State /BaseType=IntFlag/ {
        StateFlag_Active        /PyName=Flag_Active/,
        StateFlag_Busy          /PyName=Flag_Busy/,
        StateFlag_Data          /PyName=Flag_Data/,
        StateFlag_Tx            /PyName=Flag_Tx/,

        State_Disabled          /PyName=Disabled/,
        State_Idle              /PyName=Idle/,
        State_Waiting           /PyName=Waiting/,
        State_Addr              /PyName=Addr/,
        State_Addr_Busy         /PyName=Addr_Busy/,
        State_TX                /PyName=TX/,
        State_TX_Req            /PyName=TX_Req/,
        State_TX_Busy           /PyName=TX_Busy/,
        State_TX_Ack            /PyName=TX_Ack/,
        State_RX                /PyName=RX/,
        State_RX_Req            /PyName=RX_Req/,
        State_RX_Busy           /PyName=RX_Busy/,
        State_RX_Ack            /PyName=RX_Ack/,
    };

    TWI();

    void init(CycleManager&, Logger&);
    void reset();
    Signal& signal();

    void set_master_enabled(bool);
    void set_bit_delay(cycle_count_t);
    bool start_transfer();
    void end_transfer();
    bool send_address(uint8_t, bool);
    bool start_master_tx(uint8_t);
    bool start_master_rx();
    void set_master_ack(bool);

    void set_slave_enabled(bool);
    bool start_slave_tx(uint8_t);
    bool start_slave_rx();
    void set_slave_ack(bool);

    TWI::State master_state() const;
    TWI::State slave_state() const;

protected:

    virtual void packet(TWIPacket&);
    virtual void packet_ended(TWIPacket&);
    virtual void bus_acquired();
    virtual void bus_released();

};
