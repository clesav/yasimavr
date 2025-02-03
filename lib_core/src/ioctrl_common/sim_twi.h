/*
 * sim_twi.h
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

#ifndef __YASIMAVR_TWI_H__
#define __YASIMAVR_TWI_H__

#include "../core/sim_signal.h"
#include "../core/sim_cycle_timer.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================
/**
   \file
   \defgroup api_twi Two Wire Interface framework

    This code (tentatively) defines a simulation of a TWI (a.k.a. I2C or SMBus) implementation.

    It supports multiple hosts/clients, arbitration and bus collision detections. Hoever,
    it is not multi-thread safe.

    It is implemented by 3 classes:
     - EndPoint is an abstract interface defining a generic device connected to a TWI bus.
     - Client : Basic state machine & transitions for a client side interface.
     - Host : Basic state machine & transitions for a host side interface.

    These classes are abstract, the way classes access the SCL and SDA line must be defined by
    reimplementation by overriding `set_line_state()` and using `line_state_changed()`.

    Client and Host are designed to be controlled by an upper level object (a controller).
    The controller gets notified or events on the bus by signalling, and shall use the interface API
    to react accordingly.
   @{
 */

/**
   \name Controller requests definition for SPI
   @{
 */

 /**
   Request to inject a TWI bus error.
    - data->index is 0 for the host side, 1 for the client side.
 */
#define AVR_CTLREQ_TWI_BUS_ERROR     (AVR_CTLREQ_BASE + 1)

/// @}
/// @}


//=======================================================================================

/**
   \ingroup api_twi
   \brief Common enums and signal definitions for TWI classes
   \sa EndPoint, Client, Host
 */
namespace TWI {

enum Line {
    Line_Clock = 0,
    Line_Data
};

enum BusState {
    Bus_Idle,
    Bus_Busy,
    Bus_Owned
};

/**
   SignalIds common to both Client (C) and Host (H).
 */
enum SignalId {
    /**
       H+C : raised at every change of state of the interface. For debug purpose.
       data is the State enum value of the respective interface.
     */
    Signal_StateChanged = 0,

    /**
       H: raised when a change of bus state (busy/owned/idle) is detected.
       C: raised when selected/deselected by the host.
       `data` is the BusState enum value
     */
    Signal_BusStateChanged,

    /**
       C only : raised when a Start or Repeated Start condition is detected.
       `data` is 1 if the client was active prior to the Start condition, or 0
	   if it was idle.
     */
    Signal_Start,

    /**
       H only : raised after a Start or Repeated Start condition, the host is ready to transmit
       an address byte.
     */
    Signal_AddressStandby,

    /**
       H only : raised after transmitting an address byte and ACK bit has been received.
       data is the ack value: 1 for ACK, 0 for NACK.
     */
    Signal_AddressSent,

    /**
       C only : raised after receiving an address byte.
       data is the byte value.
     */
    Signal_AddressReceived,

    /**
       H+C : raised when ready to transmit/receive data.
     */
    Signal_DataStandby,

    /**
       H+C : raised when data has been sent.
     */
    Signal_DataSent,

    /**
       H+C : raised when the ACK bit after transmitting data has been received.
       data is the ack value: 1 for ACK, 0 for NACK.
     */
    Signal_DataAckReceived,

    /**
       H+C : raised when data has been received.
       data is the byte value.
     */
    Signal_DataReceived,

    /**
       H+C : raised when the ACK bit after receiving data has been sent.
     */
    Signal_DataAckSent,

    /**
       H only : raised when arbitration loss has been detected.
       data is the enum value of the state where the arbitration loss occurred.
     */
    Signal_ArbitrationLost,

    /**
       C only : raised when a bus collision has been detected.
     */
    Signal_BusCollision,

    /**
       H : raised when the transmission of a Stop condition is complete.
       C : raised when a Stop condition has been detected. `data` is 1 if
           the client was active on the bus until the stop condition and 0
           if it was idle.
     */
    Signal_Stop,
};


/**
   \ingroup api_twi
   \brief An endpoint connected to a TWI bus.
   Represents a device connected to a TWI bus model and acting as a host, a client or both.
   \sa Client, Host
 */
class AVR_CORE_PUBLIC_API EndPoint {

public:

    EndPoint();
    virtual ~EndPoint() = default;

    void line_state_changed(TWI::Line line, bool dig_state);

    bool get_clock_drive() const;
    bool get_data_drive() const;

protected:

    virtual void clock_level_changed(bool level) = 0;
    virtual void data_level_changed(bool level) = 0;
    virtual void set_line_state(TWI::Line line, bool dig_state) = 0;

    void set_clock_drive(bool level);
    bool get_clock_level() const;
    void set_data_drive(bool level);
    bool get_data_level() const;

private:

    bool m_clock_drive;
    bool m_clock_level;
    bool m_data_drive;
    bool m_data_level;

};


inline bool EndPoint::get_clock_drive() const
{
    return m_clock_drive;
}

inline bool EndPoint::get_clock_level() const
{
    return m_clock_level;
}

inline bool EndPoint::get_data_drive() const
{
    return m_data_drive;
}

inline bool EndPoint::get_data_level() const
{
    return m_data_level;
}


//=======================================================================================

/**
   \ingroup api_twi
   \brief Base abstract definition for a TWI client.
   \sa EndPoint, Host
   This class implements the basic state machine to interface a TWI bus as a client.
   It is design to be controlled by a upper layer object (a controller). The interface
   notifies the controller of bus events (start, address, etc) via the signals and
   the controller shall use the API of this class to react accordingly.
 */
class AVR_CORE_PUBLIC_API Client : public EndPoint, public CycleTimer {

public:

    enum State {
        /// Client disabled
        State_Disabled = 0,
        /// Client idle
        State_Idle,
        /// Receiving a Start condition
        State_Start,
        /// Receving a Address/RW byte
        State_AddressRx,
        /// Pending/transmitting a ACK/NACK for a read request
        State_AddressRAck,
        /// Pending/transmitting a ACK/NACK for a Write request
        State_AddressWAck,
        /// Read request ACKed, in TX mode, pending/transmitting data
        State_DataTx,
        /// Data sent, receiving ACK bit from the host
        State_DataTxAck,
        /// Write request ACKed, in RX mode, pending/receiving data
        State_DataRx,
        /// Data received, pending/sending ACK bit
        State_DataRxAck,

        /// Total number of states
        State_Count,
    };

    Client();

    void init(CycleManager& cycle_manager);

    State state() const;

    void set_enabled(bool enabled);
    bool enabled() const;

    void reset();

    bool active() const;
    bool clock_hold() const;
    unsigned char rw() const;
    bool ack() const;

    bool set_ack(bool ack);
    bool start_data_tx(uint8_t data);
    bool start_data_rx();

    Signal& signal();

    virtual cycle_count_t next(cycle_count_t when) override;

protected:

    virtual void clock_level_changed(bool level) override;
    virtual void data_level_changed(bool level) override;

private:

    State m_state;
    uint8_t m_shifter;
    bool m_ack;
    int m_bitcount;
    bool m_hold;
    CycleManager* m_cycle_manager;
    int m_deferred_drive;
    Signal m_signal;

    void set_state(State state);
    void defer_clock_release();
    void defer_data_drive(bool level);

};

/**
   Getter for the state of the client.
 */
inline Client::State Client::state() const
{
    return m_state;
}

/**
   Returns true if the client is enabled, false if disabled.
 */
inline bool Client::enabled() const
{
    return m_state != State_Disabled;
}

/**
   Returns the latest state of the ACK bit, either sent by this client, or received
   by the host, after an address or a data byte.
   \return true for ACK, false for NACK
 */
inline bool Client::ack() const
{
    return m_ack;
}

/**
   Getter for the client signal.
 */
inline Signal& Client::signal()
{
    return m_signal;
}


//=======================================================================================

/**
   \ingroup api_twi
   \brief Base abstract definition for a TWI host.
   \sa EndPoint, Client
   This class implements the basic state machine to interface a TWI bus as a host.
   It is design to be controlled by a upper layer object (a controller). The interface
   notifies the controller of bus events (start, address, etc) via the signals and
   the controller shall use the API of this class to react accordingly.
 */
class AVR_CORE_PUBLIC_API Host : public EndPoint, public CycleTimer {

public:

    enum State {
        /// Host disabled
        State_Disabled = 0,
        /// Host idle
        State_Idle,
        /// Sending a START condition
        State_Start,
        /// Sending an address/RW byte
        State_AddressTx,
        /// Waiting for a ACK/NACK bit after an address/RW byte
        State_AddressAck,
        /// Sending a data byte
        State_DataTx,
        /// Waiting for a ACK/NACK bit after sending a data byte
        State_DataTxAck,
        /// Receiving a data byte
        State_DataRx,
        /// Sending a ACK/NACK bit after receiving a data byte
        State_DataRxAck,
        /// Sending a STOP condition
        State_Stop,
        /// START condition by another host detected on the bus
        State_BusBusy,
        /// Arbitration lost
        State_ArbLost,

        /// Total number of states
        State_Count
    };

    Host();

    void init(CycleManager& manager);

    State state() const;

    void set_enabled(bool enabled);
    bool enabled() const;

    void reset();

    void set_bit_delay(cycle_count_t delay);

    bool bus_busy() const;
    bool active() const;
    bool clock_hold() const;
    bool clock_stretched() const;
    unsigned char rw() const;
    bool ack() const;

    bool start_transfer();
    bool set_address(uint8_t addr_rw);
    bool start_data_tx(uint8_t data);
    bool start_data_rx();
    bool set_ack(bool ack);
    bool stop_transfer();

    Signal& signal();

    virtual cycle_count_t next(cycle_count_t when) override;

protected:

    virtual void clock_level_changed(bool level) override;
    virtual void data_level_changed(bool level) override;

private:

    State m_state;
    uint16_t m_flags;
    uint8_t m_shifter;
    int m_step;
    int m_bitcount;
    uint8_t m_addr_rw;
    bool m_ack;
    cycle_count_t m_step_delay;
    uint8_t m_pattern;
    CycleManager* m_cycle_manager;
    bool m_hold;
    Signal m_signal;

    void set_state(State state);
    void apply_pattern();
    void process_state_and_reschedule();
    bool process_state(bool inc_step);
    void transition_state();

};

/**
   Returns true if the host is enabled, false if disabled.
 */
inline bool Host::enabled() const
{
    return m_state != State_Disabled;
}

/**
   Getter for the state of the host.
 */
inline Host::State Host::state() const
{
    return m_state;
}

/**
   Returns the latest state of the ACK bit, either sent by this host, or received
   by a client, after an address or a data byte.
   \return true for ACK, false for NACK
 */
inline bool Host::ack() const
{
    return m_ack;
}

/**
   Returns the direction of the current request, depending on the latest RW bit sent.
   \return 1 for a Read Request, 0 for a Write Request
 */
inline unsigned char Host::rw() const
{
    return m_addr_rw & 0x01;
}

/**
   Returns whether the host is currently holding the bus clock.
 */
inline bool Host::clock_hold() const
{
    return m_hold;
}

/**
   Getter for the host signal.
 */
inline Signal& Host::signal()
{
    return m_signal;
}

}; //namespace TWI


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_TWI_H__
