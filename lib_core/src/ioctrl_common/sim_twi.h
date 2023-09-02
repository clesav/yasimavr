/*
 * sim_twi.h
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

#ifndef __YASIMAVR_TWI_H__
#define __YASIMAVR_TWI_H__

#include "../core/sim_types.h"
#include "../core/sim_signal.h"
#include "../core/sim_cycle_timer.h"
#include "../core/sim_device.h"
#include <deque>
#include <vector>

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/*
 * This code (tentatively) defines a simulation of a TWI (a.k.a. I2C or SMBus) implementation
 * It supports multiple masters/slaves.
 * It does not support arbitration beyond the START condition and (currently) is not
 * multi-thread safe.
 * It is implemented by 4 classes:
 *  - TWIPacket defines a packet circulating on a bus simulating the successive exchange
 *    of information between mater and slave.
 *  - TWIEndPoint is an abstract interface defining a generic device connected to a TWI bus.
 *  - TWIBus defines a central object to circulate packets between multiple endpoints.
 *  - TWI is an implementation of a TWI interface for an AVR MCU, as generic
 *    as possible. It manages master and slave operations independently and communicates
 *    with upper layers via signals.
 */


/*
 * CTLREQ definitions
*/
//Request to get the TWI endpoint. data->p must be pointing at a TWIEndPoint
#define AVR_CTLREQ_TWI_ENDPOINT     1


//=======================================================================================

class AVR_CORE_PUBLIC_API TWIPacket {

public:

    //This enum lists the types of packet.
    //Address, WriteData and ReadData (and ReadRequest under some conditions)
    //are 'long' packets, i.e. they have a 'duration' that is simulated by a
    //'send' and a 'end' callback.
    //The xxxxAck packets are 'short', i.e. instantaneous. In reality they have
    //a duration but it's included in the respective 'long' packet duration.
    enum Cmd {
        Cmd_Invalid = 0,
        Cmd_Address,
        Cmd_AddrAck,
        Cmd_DataRequest,
        Cmd_Data,
        Cmd_DataAck,
    };

    enum {
        Nack = 0,
        Ack = 1,
        Write = 0,
        Read = 1,
    };

    uint32_t cmd : 3,
             addr : 7,
             rw : 1,
             data : 8,
             ack: 1,
             hold: 1,
             unused: 11;

    TWIPacket();

};

class TWIBus;

class AVR_CORE_PUBLIC_API TWIEndPoint {

public:

    TWIEndPoint();
    virtual ~TWIEndPoint();

    inline TWIBus* bus() const { return m_bus; }

    //Disable copy semantics
    TWIEndPoint(const TWIEndPoint&) = delete;
    TWIEndPoint& operator=(const TWIEndPoint&) = delete;

protected:

    //Used by a master endpoint to get ownership of the bus
    bool acquire_bus();
    //Used by a master endpoint to release ownship of the bus
    void release_bus();
    //Used by a endpoint to send a packet on the bus
    void send_packet(TWIPacket& packet);
    //Used by a endpoint to end a 'long' packet
    void end_packet(TWIPacket& packet);

    //*********************************

    //Called by the bus to transmit a packet.
    virtual void packet(TWIPacket& packet) = 0;
    virtual void packet_ended(TWIPacket& packet) = 0;
    //Called by the bus to signal that the bus is acquired
    virtual void bus_acquired() = 0;
    //Called by the bus to signal that the bus is released
    virtual void bus_released() = 0;

private:

    friend class TWIBus;

    TWIBus* m_bus;

};


class AVR_CORE_PUBLIC_API TWIBus {

public:

    //Signal definitions
    enum SignalId {
        //Signal emitted when the bus has been acquired by a master
        //sigdata.p is pointing at the endpoint master who acquired the bus.
        Signal_Start,
        //Signal emitted when a address packet is sent over the bus.
        //sigdata.p is pointing at the packet
        Signal_Address,
        //Signal emitted when a data packet is sent over the bus.
        //sigdata.p is pointing at the packet
        Signal_Data,
        //Signal emitted when a long packet is ended.
        //no data attached
        Signal_Packet_End,
        //Signal emitted when a ACK/NACK (address or data) is sent over the bus.
        //sigdata.p is pointing at the packet
        Signal_Ack,
        //Signal emitted when the bus is released.
        //sigdata.p is pointing at the endpoint master that owned the bus.
        Signal_Stop,
    };

    TWIBus();
    ~TWIBus();

    Signal& signal();

    void add_endpoint(TWIEndPoint& endpoint);
    void remove_endpoint(TWIEndPoint& endpoint);

    //Disable copy semantics
    TWIBus(const TWIBus&) = delete;
    TWIBus& operator=(const TWIBus&) = delete;

private:

    friend class TWIEndPoint;

    Signal m_signal;
    //List of all the endpoints connected to this bus
    std::vector<TWIEndPoint*> m_endpoints;
    //Pointer to the master currently owning the bus
    TWIEndPoint* m_master;
    //Pointer to the currently active slave
    TWIEndPoint* m_slave;
    //When a ADDR packet is circulating, and slaves delayed
    //their ACK response by holding the bus, this is the counter
    //of expected ACK.
    unsigned int m_expected_ack;

    //Called by a master who wants to acquire the bus
    //endpoint is the calling host.
    //Returns true if ownership has been granted of false if the bus
    //is busy.
    bool acquire(TWIEndPoint* endpoint);
    //Called by the bus owner to relinquish ownership of the bus
    //at the end of a transfer.
    void release(TWIEndPoint* endpoint);
    //Called by an enpoint transmitter (slave or master)
    void send_packet(TWIEndPoint& src, TWIPacket& p);
    //Called by the bus owner to notify the end of a long packet
    void end_packet(TWIEndPoint& src, TWIPacket& p);

};

inline Signal& TWIBus::signal()
{
    return m_signal;
}


class AVR_CORE_PUBLIC_API TWI : public TWIEndPoint {

public:

    //For all the signals below, sigdata.index is set to either Cpt_Master
    //or Cpt_Slave identifying which part of the endpoint is emitting the
    //signal
    enum SignalId {
        //For debug and logging purpose only
        Signal_StateChange,
        //Emitted when the bus state changed. sigdata.u is set to one
        //of the BusState enumeration values.
        Signal_BusStateChange,
        //Slave only signal. Emitted when received a address packet.
        //sigdata.u is set to the raw byte received, so
        //bit 0 is the RW flag, bits 1 to 7 contain the address
        Signal_Address,
        //Master only signal. Emitted when received a address ACK/NACK
        //from a slave. sigdata.u is set to TWIPacket::Ack or TWIPacket::Nack
        Signal_AddrAck,
        //Emitted when a data transmission complete, i.e. the ACK/NACK has been
        //received in return. sigdata.u is set to TWIPacket::Ack or TWIPacket::Nack.
        Signal_TxComplete,
        //Emitted when a data reception completed. sigdata.u is set to the data byte
        //received. A ACK/NACK has not been sent in return yet.
        Signal_RxComplete,
    };

    enum Component {
        Cpt_Any,
        Cpt_Master,
        Cpt_Slave,
    };

    enum BusState {
        //The bus is idle
        Bus_Idle,
        //The bus is owned by another master
        Bus_Busy,
        //The bus is owned by this instance
        Bus_Owned,
    };

    //State enumeration values are split in 2 nibbles :
    //bits 0 to 3 : a OR'ed combination of StateFlag enum values
    //bits 4 to 7 : a integer incremented only to differentiate the values
    enum State {
        //Active here means actively participating in bus traffic
        //either as master or as slave
        StateFlag_Active    = 0x01,
        StateFlag_Busy      = 0x02,
        StateFlag_Data      = 0x04,
        StateFlag_Tx        = 0x08,

        State_Disabled      = 0x00,
        State_Idle          = 0x10,
        //Waiting for the bus to be released
        State_Waiting       = 0x20,
        //Pending a ADDR packet, where the next valid actions are either
        //a STOP (bus release) or a RESTART (new Address packet)
        State_Addr          = 0x31,
        //Address packet transmitting/receiving or waiting for ACK/NACK
        State_Addr_Busy     = 0x43,
        //Slave address ACKed, in TX mode, ready to send
        State_TX            = 0x5D,

        State_TX_Req        = 0x6F,
        //Sending data in progress
        State_TX_Busy       = 0x7F,
        //Waiting for a TX ACK/NACK
        State_TX_Ack        = 0x8F,
        //Slave address ACKed, in RX mode, ready to receive
        State_RX            = 0x95,
        //Read data request sent, waiting for data packet (master only)
        State_RX_Req        = 0xA7,
        //Receiving data in progress
        State_RX_Busy       = 0xB7,
        //Waiting for a RX ACK/NACK
        State_RX_Ack        = 0xC7,
    };

    TWI();
    virtual ~TWI();

    //Initialise the interface. the device will be used for timer related operations
    void init(CycleManager& cycle_manager, Logger& logger);

    //Reset the interface cancel any transaction
    void reset();

    Signal& signal();

    void set_master_enabled(bool enabled);
    void set_bit_delay(cycle_count_t delay);
    bool start_transfer();
    void end_transfer();
    bool send_address(uint8_t remote_addr, bool rw);

    //(Master only) sends a byte of data to the slave.
    //Returns true if the data could actually be sent, false otherwise.
    bool start_master_tx(uint8_t data);

    //(Master only) sends a ReadRequest packet to the slave
    //Returns true if the packet could actually be sent, false otherwise
    bool start_master_rx();

    //Used when the slave is expecting a ACK/NACK response
    //after a ReadData packet
    //ack: true=ACK, false=NACK
    void set_master_ack(bool ack);

    //Enable/disable the slave component
    void set_slave_enabled(bool enabled);

    //(Slave only) Sends a byte of data to the master.
    //Returns true if the data could actually been sent, false otherwise
    bool start_slave_tx(uint8_t data);

    //(Slave only) Indicates to the master that the slave is ready to receive
    //data. Returns True if the operation was legal, false otherwise.
    bool start_slave_rx();

    //Function used when the master is expected a ACK/NACK response
    //after a Address or a WriteData packet
    //ack: true=ACK, false=NACK
    void set_slave_ack(bool ack);

    State master_state() const;
    State slave_state() const;

    //Disable copy semantics
    TWI(const TWI&) = delete;
    TWI& operator=(const TWI&) = delete;

protected:

    virtual void packet(TWIPacket& packet) override;
    virtual void packet_ended(TWIPacket& packet) override;
    virtual void bus_acquired() override;
    virtual void bus_released() override;

private:

    class Timer;
    friend class Timer;

    CycleManager* m_cycle_manager;
    Logger* m_logger;

    Signal m_signal;
    signal_data_t m_deferred_sigdata;
    bool m_has_deferred_raise;
    Timer* m_timer;
    bool m_timer_updating;
    cycle_count_t m_timer_next_when;

    TWIPacket m_current_packet;

    uint8_t m_tx_data;

    State m_mst_state;
    cycle_count_t m_bitdelay;

    State m_slv_state;
    bool m_slv_hold;

    void set_master_state(State newstate);
    void set_slave_state(State newstate);

    void start_timer(cycle_count_t delay);
    cycle_count_t timer_next(cycle_count_t when);
    void defer_signal_raise(int sigid, long long index, unsigned long long u);

};

inline Signal& TWI::signal()
{
    return m_signal;
}

inline TWI::State TWI::master_state() const
{
    return m_mst_state;
}

inline TWI::State TWI::slave_state() const
{
    return m_slv_state;
}


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_TWI_H__
