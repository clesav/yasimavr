/*
 * sim_twi.cpp
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

#include "sim_twi.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

TWI_Packet::TWI_Packet()
:cmd(Cmd_Invalid)
,addr(0)
,rw(0)
,data(0)
,ack(Nack)
,hold(0)
,unused(0)
{}

static void fill_address_packet(TWI_Packet& packet, uint8_t a, bool rw_)
{
    packet.cmd = TWI_Packet::Cmd_Address;
    packet.addr = a;
    packet.rw = rw_ ? TWI_Packet::Read : TWI_Packet::Write;
    packet.data = 0;
    packet.ack = TWI_Packet::Nack;
    packet.hold = 0;
}

static void fill_addr_ack_packet(TWI_Packet& packet, bool ack)
{
    packet.cmd = TWI_Packet::Cmd_AddrAck;
    packet.ack = ack ? TWI_Packet::Ack : TWI_Packet::Nack;
}

static void fill_write_req_packet(TWI_Packet& packet, uint8_t d)
{
    packet.cmd = TWI_Packet::Cmd_DataRequest;
    packet.rw = TWI_Packet::Write;
    packet.data = d;
    packet.ack = TWI_Packet::Nack;
    packet.hold = 0;
}

static void fill_write_packet(TWI_Packet& packet)
{
    packet.cmd = TWI_Packet::Cmd_Data;
    packet.rw = TWI_Packet::Write;
    packet.data = 0;
    packet.ack = TWI_Packet::Nack;
    packet.hold = 0;
}

static void fill_read_req_packet(TWI_Packet& packet)
{
    packet.cmd = TWI_Packet::Cmd_DataRequest;
    packet.rw = TWI_Packet::Read;
    packet.data = 0;
    packet.ack = TWI_Packet::Nack;
    packet.hold = 0;
}

 void fill_read_packet(TWI_Packet& packet, uint8_t d)
 {
    packet.cmd = TWI_Packet::Cmd_Data;
    packet.rw = TWI_Packet::Read;
    packet.data = 0;
    packet.ack = TWI_Packet::Nack;
    packet.hold = 0;
 }


//=======================================================================================

TWI_Endpoint::TWI_Endpoint()
:m_bus(nullptr)
{}

TWI_Endpoint::~TWI_Endpoint()
{
    if (m_bus)
        m_bus->remove_endpoint(*this);
}

bool TWI_Endpoint::acquire_bus()
{
    if (m_bus)
        return m_bus->acquire(this);
    else
        return false;
}

void TWI_Endpoint::release_bus()
{
    if (m_bus)
        m_bus->release(this);
}

void TWI_Endpoint::send_packet(TWI_Packet& packet)
{
    if (m_bus)
        m_bus->send_packet(*this, packet);
}

void TWI_Endpoint::end_packet(TWI_Packet& packet)
{
    if (m_bus)
        m_bus->end_packet(*this, packet);
}


//=======================================================================================

TWI_Bus::TWI_Bus()
:m_master(nullptr)
,m_slave(nullptr)
,m_expected_ack(0)
{}

TWI_Bus::~TWI_Bus()
{
    for (TWI_Endpoint* endpoint : m_endpoints)
        endpoint->m_bus = nullptr;
}

void TWI_Bus::add_endpoint(TWI_Endpoint& endpoint)
{
    if (!m_master) {
        for (auto it = m_endpoints.begin(); it != m_endpoints.end(); ++it) {
            if (*it == &endpoint) return;
        }

        m_endpoints.push_back(&endpoint);
        endpoint.m_bus = this;
    }
}

void TWI_Bus::remove_endpoint(TWI_Endpoint& endpoint)
{
    if (!m_master) {
        for (auto it = m_endpoints.begin(); it != m_endpoints.end(); ++it) {
            if (*it == &endpoint) {
                m_endpoints.erase(it);
                endpoint.m_bus = nullptr;
                break;
            }
        }
    }
}

bool TWI_Bus::acquire(TWI_Endpoint* endpoint)
{
    m_slave = nullptr;
    if (m_master && m_master != endpoint) {
        return false;
    } else {
        m_master = endpoint;
        m_signal.raise(Signal_Start, endpoint);

        for (auto ep: m_endpoints) {
            if (ep != endpoint)
                ep->bus_acquired();
        }

        return true;
    }
}

void TWI_Bus::release(TWI_Endpoint* endpoint)
{
    if (endpoint == m_master) {
        m_master = nullptr;
        m_slave = nullptr;
        m_signal.raise(Signal_Stop, endpoint);

        for (auto ep: m_endpoints) {
            if (ep != endpoint)
                ep->bus_released();
        }
    }
}

void TWI_Bus::send_packet(TWI_Endpoint& src, TWI_Packet& packet)
{
    switch(packet.cmd) {

        case TWI_Packet::Cmd_Address: {
            for (auto endpoint : m_endpoints)
                endpoint->packet(packet);

            //Raise the bus signal
            m_signal.raise(Signal_Address, &packet);

        } break;

        case TWI_Packet::Cmd_AddrAck: {

            //if not expecting any further AddrAck, the packet can be ignored
            if (m_expected_ack) {
                --m_expected_ack;
                //If the source endpoint has set the ack flag, select it as the
                //active slave
                if (packet.ack == TWI_Packet::Ack)
                    m_slave = &src;
                //If there's a selected slave or this was the last expected ack
                //the packet can be forwarded to the master
                if (m_slave || !m_expected_ack) {
                    m_master->packet(packet);
                    m_expected_ack = 0;
                    m_signal.raise(Signal_Ack, &packet);
                }
            }

        } break;

        case TWI_Packet::Cmd_DataRequest:
            m_slave->packet(packet);
            if (!packet.hold)
                m_signal.raise(Signal_Data, &packet);
            break;

        case TWI_Packet::Cmd_Data:
            m_master->packet(packet);
            m_signal.raise(Signal_Data, &packet);
            break;

        case TWI_Packet::Cmd_DataAck:
            if (&src == m_master)
                m_slave->packet(packet);
            else
                m_master->packet(packet);

            m_signal.raise(Signal_Ack, &packet);
            break;

    }
}

void TWI_Bus::end_packet(TWI_Endpoint& src, TWI_Packet& packet)
{
    switch(packet.cmd) {

        case TWI_Packet::Cmd_Address: {

            //Forward the packet to every endpoint on the bus,
            //including the source, in case it wants to do a loopback
            m_expected_ack = m_endpoints.size();
            m_slave = nullptr;
            for (auto endpoint : m_endpoints) {
                //Send the endpoint a temporary copy of the current packet.
                //The endpoint should configure the 'ack' and 'hold' bits
                TWI_Packet p = packet;
                endpoint->packet_ended(p);

                //If the receiving endpoint doesn't want to hold the bus
                if (!p.hold) {
                    //that's one less AddrAck packet to expect
                    if (m_expected_ack)
                        --m_expected_ack;

                    //If the receiving endpoint has set the ack flag,
                    //select it as the active slave
                    if (!m_slave && p.ack == TWI_Packet::Ack) {
                        m_slave = endpoint;
                        packet.ack = TWI_Packet::Ack;
                        packet.hold = 0;
                        //We don't need to process the AddrAck packets
                        //from the other endpoint
                        m_expected_ack = 0;
                    }
                }
            }

            //If no slave selected yet, reply with Nack and if
            //further AddrAck packets are expected, hold the bus
            if (!m_slave) {
                packet.ack = TWI_Packet::Nack;
                packet.hold = m_expected_ack ? 1 : 0;
            }

        } break;

        case TWI_Packet::Cmd_DataRequest:
            m_slave->packet_ended(packet);
            break;

        case TWI_Packet::Cmd_Data:
            m_slave->packet_ended(packet);
            break;
    }

    //Raise the bus signal, notifying the end of the packet
    m_signal.raise(Signal_Packet_End);
}


//=======================================================================================

class IO_TWI::Timer : public CycleTimer {

public:

    Timer(IO_TWI& ctl) : m_ctl(ctl) {}

    virtual cycle_count_t next(cycle_count_t when) override
    {
        return m_ctl.timer_next(when);
    }

private:

    IO_TWI& m_ctl;

};


inline bool State_Active(IO_TWI::State state)
{
    return (state & IO_TWI::StateFlag_Active);
}

inline bool State_Busy(IO_TWI::State state)
{
    return (state & IO_TWI::StateFlag_Busy);
}

inline bool State_Data(IO_TWI::State state)
{
    return (state & IO_TWI::StateFlag_Data);
}

inline bool State_Tx(IO_TWI::State state)
{
    return (state & IO_TWI::StateFlag_Tx);
}


IO_TWI::IO_TWI()
:m_cycle_manager(nullptr)
,m_logger(nullptr)
,m_has_deferred_raise(false)
,m_timer_updating(false)
,m_timer_next_when(0)
,m_tx_data(0)
,m_mst_state(State_Disabled)
,m_bitdelay(1)
,m_slv_state(State_Disabled)
,m_slv_hold(false)
{
    m_timer = new Timer(*this);
}

IO_TWI::~IO_TWI()
{
    delete m_timer;
}

void IO_TWI::init(CycleManager& cycle_manager, Logger& logger)
{
    m_cycle_manager = &cycle_manager;
    m_logger = &logger;
}

void IO_TWI::reset()
{
    m_has_deferred_raise = false;

    if (State_Active(m_mst_state))
        release_bus();

    m_timer_updating = 0;
    m_timer_next_when = 0;

    m_mst_state = State_Disabled;
    m_bitdelay = 1;
    m_cycle_manager->cancel(*m_timer);

    m_slv_state = State_Disabled;
    m_slv_hold = false;
}

//=======================================================================================
/*
 * Master operations
 */

void IO_TWI::set_master_enabled(bool enabled)
{
    if (m_mst_state != State_Disabled && !enabled) {
        if (State_Active(m_mst_state))
            release_bus();

        m_cycle_manager->cancel(*m_timer);

        set_master_state(State_Disabled);
    }
    else if (m_mst_state == State_Disabled && enabled && bus()) {
        set_master_state(State_Idle);
    }
}

void IO_TWI::set_master_state(State new_state)
{
    m_mst_state = new_state;
    m_signal.raise_u(Signal_StateChange, new_state, Cpt_Master);
}

void IO_TWI::set_bit_delay(cycle_count_t delay)
{
    m_bitdelay = delay;
}

bool IO_TWI::start_transfer()
{
    //Illegal to be here if not idle
    if (m_mst_state != State_Idle)
        return false;

    //try to acquire the bus ownership
    if (acquire_bus()) {
        m_logger->dbg("Ownership of bus acquired");
        set_master_state(State_Addr);
        m_signal.raise_u(Signal_BusStateChange, Bus_Owned, Cpt_Any);
        return true;
    } else {
        set_master_state(State_Waiting);
        m_signal.raise_u(Signal_BusStateChange, Bus_Busy, Cpt_Any);
        return false;
    }
}

bool IO_TWI::send_address(uint8_t remote_addr, bool rw)
{
    if (!State_Active(m_mst_state) || State_Busy(m_mst_state))
        return false;

    set_master_state(State_Addr_Busy);

    //Prepare and send the address packet
    fill_address_packet(m_current_packet, remote_addr, rw);
    send_packet(m_current_packet);

    start_timer(m_bitdelay * 9); //ADDR (7 bits) + RW + ACK

    return true;
}

void IO_TWI::end_transfer()
{
    if (State_Active(m_mst_state) && !State_Busy(m_mst_state)) {
        set_master_state(State_Idle);
        m_signal.raise_u(Signal_BusStateChange, Bus_Idle, Cpt_Any);
        release_bus();
    }
}

bool IO_TWI::start_master_tx(uint8_t data)
{
    if (m_mst_state != State_TX)
        return false;

    m_tx_data = data;

    //Prepare and send a DataRequest packet
    fill_write_req_packet(m_current_packet, data);
    send_packet(m_current_packet);
    //If the slave has set the hold bit, we need to wait
    //for a Data packet later
    if (m_current_packet.hold) {
        set_master_state(State_TX_Req);
    } else {
        set_master_state(State_TX_Busy);
        start_timer(m_bitdelay * 9); //DATA (8 bits) + ACK
    }

    return true;
}

bool IO_TWI::start_master_rx()
{
    if (m_mst_state != State_RX)
        return false;

    //Send a DataRequest packet
    fill_read_req_packet(m_current_packet);
    send_packet(m_current_packet);

    if (m_current_packet.hold) {
        //If hold is set, it means the slave will send a ReadData packet later
        set_master_state(State_RX_Req);
    } else {
        //If hold is clear, the slave will put the data in the
        //ReadRequest packet when end_packet is called
        set_master_state(State_RX_Busy);
        start_timer(m_bitdelay * 9); //DATA (8 bits) + ACK
    }

    return true;
}

void IO_TWI::set_master_ack(bool ack)
{
    if (m_mst_state == State_RX_Ack) {
        //Prepare and send the ReadAck packet
        m_current_packet.cmd = TWI_Packet::Cmd_DataAck;
        m_current_packet.ack = ack ? TWI_Packet::Ack : TWI_Packet::Nack;
        send_packet(m_current_packet);
        //If acked, transit to RX to read the next data byte
        //If nacked, the only valid next moves are a new Address packet
        //or a transaction end
        set_master_state(ack ? State_RX : State_Addr);
    }
}

void IO_TWI::start_timer(cycle_count_t delay)
{
    if (m_timer_updating)
        m_timer_next_when = m_cycle_manager->cycle() + delay;
    else
        m_cycle_manager->delay(*m_timer, delay);
}

cycle_count_t IO_TWI::timer_next(cycle_count_t when)
{
    m_timer_updating = true;
    m_timer_next_when = 0;

    //Process any deferred signal to raise
    //The reason signals are deferred when raised outside the cycle timer callback
    //is to avoid TWI_Bus callbacks directly calling TWI_Bus functions as it
    //may lead to infinite loops or deadlocks
    if (m_has_deferred_raise) {
        m_logger->dbg("Deferred signal raise, id=%d", m_deferred_sigdata.sigid);
        m_signal.raise(m_deferred_sigdata);
        m_has_deferred_raise = false;
    }

    else if (m_mst_state == State_Addr_Busy) {
        //We're here at the end of an Address packet
        end_packet(m_current_packet);
        //If the hold bit is set, we need to wait for an AddrAck packet,
        //otherwise we can process the ack and rw bits and signal the upper layer
        if (!m_current_packet.hold) {
            if (!m_current_packet.ack)
                set_master_state(State_Addr);
            else if (m_current_packet.rw)
                set_master_state(State_RX);
            else
                set_master_state(State_TX);

            m_signal.raise_u(Signal_AddrAck, m_current_packet.ack, Cpt_Master);
        }
    }

    else if (m_mst_state == State_TX_Busy) {
        //We're here at the end of a Data or long DataRequest packet
        m_current_packet.data = m_tx_data;
        end_packet(m_current_packet);
        //If the slave has set the hold bit, it means it will send a DataAck packet
        //later.
        //If cleared, the ack bit is valid and is signalled to the upper layer
        if (m_current_packet.hold) {
            set_master_state(State_TX_Ack);
        } else {
            set_master_state(State_TX);
            m_signal.raise_u(Signal_TxComplete, m_current_packet.ack, Cpt_Master);
        }
    }

    else if (m_mst_state == State_RX_Busy) {
        //We're here at the end of a Data or a long DataRequest packet
        //For ReadData, by setting hold, we inform the slave that the master
        //will send a ACK/NACK later. No effect for ReadRequest packets.
        m_current_packet.hold = 1;
        end_packet(m_current_packet);
        //The packet now contains the data byte provided by the slave
        //Setting the ACK/NACK is the responsibility of the upper layers
        set_master_state(State_RX_Ack);
        m_signal.raise_u(Signal_RxComplete, m_current_packet.data, Cpt_Master);
    }

    else
        abort();

    m_timer_updating = false;

    return m_timer_next_when;
}

void IO_TWI::defer_signal_raise(uint16_t sigid, uint32_t index, uint32_t u)
{
    signal_data_t sig = { .sigid = sigid, .index = index, .data = u };
    m_deferred_sigdata = sig;
    m_has_deferred_raise = true;
    start_timer(1);
}

//=======================================================================================
/*
 * Slave operations
 */

void IO_TWI::set_slave_enabled(bool enabled)
{
    if (m_slv_state != State_Disabled && !enabled) {
        TWI_Packet packet = m_current_packet;

        if (m_slv_state == State_TX_Req) {
            fill_read_packet(packet, 0xFF);
            send_packet(packet);
        }
        else if (m_slv_state == State_RX_Req) {
            fill_write_packet(packet);
            send_packet(packet);
        }
        else if (m_slv_state == State_RX_Ack) {
            packet.cmd = TWI_Packet::Cmd_DataAck;
            packet.ack = TWI_Packet::Nack;
            send_packet(packet);
        }
        else if (m_slv_state == State_Addr_Busy && m_slv_hold) {
            fill_addr_ack_packet(packet, false);
            send_packet(packet);
        }

        set_slave_state(State_Disabled);
    }
    else if (m_slv_state == State_Disabled && enabled) {
        set_slave_state(State_Idle);
    }
}

void IO_TWI::set_slave_state(State new_state)
{
    m_slv_state = new_state;
    m_signal.raise_u(Signal_StateChange, new_state, Cpt_Slave);
}

bool IO_TWI::start_slave_tx(uint8_t data)
{
    if (m_slv_state == State_TX) {
        m_tx_data = data;
        m_slv_hold = false;
        return true;
    }
    else if (m_slv_state == State_TX_Req) {
        m_tx_data = data;
        m_slv_hold = false;
        TWI_Packet packet = m_current_packet;
        fill_read_packet(packet, data);
        send_packet(packet);
        set_slave_state(State_TX_Busy);
        return true;
    }
    else {
        return false;
    }
}

bool IO_TWI::start_slave_rx()
{
    if (m_slv_state == State_RX) {
        m_slv_hold = false;
        return true;
    }
    else if (m_slv_state == State_RX_Req) {
        m_slv_hold = false;
        TWI_Packet packet = m_current_packet;
        fill_write_packet(packet);
        send_packet(packet);
        set_slave_state(State_RX_Busy);
        return true;
    }
    else {
        return false;
    }
}

void IO_TWI::set_slave_ack(bool ack)
{
    if (m_slv_state == State_Addr_Busy) {

        //In Addr_Busy state, the master is expecting an ACK/NACK for an address
        TWI_Packet packet = m_current_packet;
        packet.cmd = TWI_Packet::Cmd_AddrAck;
        packet.ack = ack ? TWI_Packet::Ack : TWI_Packet::Nack;
        send_packet(packet);

        //If acked, transit to data TX or RX state depending on the RW bit of the packet
        //If nacked, transit to Waiting state to wait until a bus release
        if (ack)
            set_slave_state(m_current_packet.rw ? State_TX : State_RX);
        else
            set_slave_state(State_Waiting);

    }

    else if (m_slv_state == State_RX_Ack) {

        //In RX_Ack state, the master is expecting an ACK/NACK after a Data Write
        TWI_Packet packet = m_current_packet;
        packet.cmd = TWI_Packet::Cmd_DataAck;
        packet.ack = ack ? TWI_Packet::Ack : TWI_Packet::Nack;
        send_packet(packet);

        //If acked, transit to RX state to wait for the next Data Write
        //if nacked, transit to Addr state to wait for a bus release or a new Address packet
        if (ack) {
            set_slave_state(State_RX);
            m_slv_hold = true;
        } else {
            set_slave_state(State_Addr);
        }

    }
}

//=======================================================================================
/*
 * Endpoint interface reimplementation
 */

void IO_TWI::packet(TWI_Packet& packet)
{
    m_logger->dbg("Packet received Command=%d", packet.cmd);

    switch(packet.cmd) {

        //Received a address packet (slave side)
        case TWI_Packet::Cmd_Address: {

            if (m_slv_state == State_Idle || State_Active(m_slv_state)) {
                //Set the state in address receiving / address match
                set_slave_state(State_Addr_Busy);
            }

        } break;

        //Received an address ack packet (master side)
        case TWI_Packet::Cmd_AddrAck: {

            if (m_mst_state == State_Addr_Busy) {
                //If no ack, go the Addr state to wait for the next
                //address to be given by the upper layer
                if (!packet.ack)
                    set_master_state(State_Addr);
                //if RW is set, it's a Read request
                else if (packet.rw)
                    set_master_state(State_RX);
                //if RW is not set, it's a Write request
                else
                    set_master_state(State_TX);

                defer_signal_raise(Signal_AddrAck, Cpt_Master, packet.ack);
            }

        } break;

        //Received a data request (slave side)
        case TWI_Packet::Cmd_DataRequest: {

            if (m_slv_state == State_TX) {
                set_slave_state(m_slv_hold ? State_TX_Req : State_TX_Busy);
                packet.hold = m_slv_hold ? 1 : 0;
            }
            else if (m_slv_state == State_RX) {
                set_slave_state(m_slv_hold ? State_RX_Req : State_RX_Busy);
                packet.hold = m_slv_hold ? 1 : 0;
            }

        } break;

        //Received a Data packet (master side)
        //Only start a timer, the actual processing
        //is done at the end of the long packet
        case TWI_Packet::Cmd_Data: {

            if (m_mst_state == State_TX_Req) {
                set_master_state(State_TX_Busy);
                start_timer(m_bitdelay * 9);
            }
            else if (m_mst_state == State_RX_Req) {
                set_master_state(State_RX_Busy);
                start_timer(m_bitdelay * 9);
            }

        } break;

        //Received a Data Ack packet (master or slave side)
        case TWI_Packet::Cmd_DataAck: {

            if (m_mst_state == State_TX_Ack) {
                //If the slave replied ACK, it's expecting another byte.
                //If it replied NACK, the transfer will end
                if (packet.ack == TWI_Packet::Ack)
                    set_master_state(State_TX);
                else
                    set_master_state(State_Addr);

                defer_signal_raise(Signal_TxComplete, Cpt_Master, packet.ack);
            }

            if (m_slv_state == State_TX_Ack) {
                //If the master replied ACK, it's expecting another byte.
                //If it replied NACK, the transfer will end
                if (packet.ack == TWI_Packet::Ack) {
                    set_slave_state(State_TX);
                    m_slv_hold = true;
                } else {
                    set_slave_state(State_Addr);
                }

                defer_signal_raise(Signal_TxComplete, Cpt_Slave, packet.ack);
            }

        } break;
    }

    m_current_packet = packet;
}

void IO_TWI::packet_ended(TWI_Packet& packet)
{
    m_logger->dbg("Packet ended, Command=%d", packet.cmd);

    //Upon receiving a packet end for an address (slave only),
    //hold the bus and send the address to the higher layer to
    //match it and provide a ACK/NACK status via 'set_slave_ack'
    if (packet.cmd == TWI_Packet::Cmd_Address) {
        if (m_slv_state == State_Addr_Busy) {
            packet.hold = 1;
            uint8_t u = (packet.addr << 1) | packet.rw;
            m_slv_hold = true;
            defer_signal_raise(Signal_Address, Cpt_Slave, u);
        }
    }

    //End of a Data or 'long' DataRequest packet. (slave only)
    //A Data packet is always long
    //If the Datarequest packet is long, it's because the hold flag
    //wasn't set in "packet()"
    else if (packet.cmd == TWI_Packet::Cmd_DataRequest ||
             packet.cmd == TWI_Packet::Cmd_Data) {

        //In TX, fill the packet data and check the hold bit that
        //indicates if the master can already ACK/NACK the data.
        if (m_slv_state == State_TX_Busy) {
            packet.data = m_tx_data;
            if (packet.hold) {
                set_slave_state(State_TX_Ack);
            } else {
                set_slave_state(State_TX);
                m_slv_hold = true;
                defer_signal_raise(Signal_TxComplete, Cpt_Slave, packet.ack);
            }
        }
        //In RX, the packet contains the data from the master.
        //The upper layer provides the ACK/NACK so here we hold the bus
        else if (m_slv_state == State_RX_Busy) {
            packet.hold = 1;
            set_slave_state(State_RX_Ack);
            defer_signal_raise(Signal_RxComplete, Cpt_Slave, packet.data);
        }

    }

    m_current_packet = packet;
}

void IO_TWI::bus_acquired()
{
    m_logger->dbg("Bus acquired");

    if (m_mst_state == State_Idle) {
        set_master_state(State_Waiting);
        defer_signal_raise(Signal_BusStateChange, Cpt_Any, Bus_Busy);
    }
}

void IO_TWI::bus_released()
{
    m_logger->dbg("Bus released");

    defer_signal_raise(Signal_BusStateChange, Cpt_Any, Bus_Idle);

    if (m_mst_state > State_Idle)
        set_master_state(State_Idle);

    if (m_slv_state > State_Idle)
        set_slave_state(State_Idle);
}
