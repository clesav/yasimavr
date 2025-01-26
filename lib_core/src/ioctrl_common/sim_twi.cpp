/*
 * sim_twi.cpp
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

#include "sim_twi.h"

YASIMAVR_USING_NAMESPACE

using namespace TWI;


enum StateFlag {
    StateFlag_BusBusy        = 0x001,
    StateFlag_Active         = 0x002,
    StateFlag_Timer          = 0x004,
    StateFlag_Hold           = 0x008,
    StateFlag_ByteShift      = 0x010,
    StateFlag_Drive          = 0x020,
    StateFlag_ArbCheck       = 0x040,
    StateFlag_ClockStretch   = 0x080,
    StateFlag_RW             = 0x100,
};


//=======================================================================================

/**
   Construction of an end point.
 */
EndPoint::EndPoint()
:m_clock_drive(true)
,m_clock_level(true)
,m_data_drive(true)
,m_data_level(true)
{}

/**
   Inform the interface of a digital state change on a line (SCL or SDA).
   The state of the interface will transition accordingly.
 */
void EndPoint::line_state_changed(Line line, bool dig_state)
{
    if (line == Line_Clock) {
        if (dig_state ^ m_clock_level) {
            m_clock_level = dig_state;
            clock_level_changed(dig_state);
        }
    } else {
        if (dig_state ^ m_data_level) {
            m_data_level = dig_state;
            data_level_changed(dig_state);
        }
    }
}

/**
   Return the level of the SCL line driven by this.
 */
void EndPoint::set_clock_drive(bool level)
{
    m_clock_drive = level;
    set_line_state(Line_Clock, level);
}

/**
   Return the level of the SDA line driven by this.
 */
void EndPoint::set_data_drive(bool level)
{
    m_data_drive = level;
    set_line_state(Line_Data, level);
}


//=======================================================================================

static const uint16_t ClientStateFlags[Client::State_Count] = {
    0x000, //Disabled:         no flag
    0x001, //Waiting:          BusBusy
    0x001, //Start:            BusBusy
    0x001, //AddressRx:        BusBusy
    0x009, //AddressRAck:      BusBusy,Hold
    0x109, //AddressWAck:      BusBusy,Hold,RW
    0x10B, //DataTx:           BusBusy,Active,Hold,RW
    0x103, //DataTxAck:        BusBusy,Active,RW
    0x00B, //DataRx:           BusBusy,Active,Hold
    0x00B, //DataRxAck:        BusBusy,Active,Hold
};


#define DEFER_CLOCK_HI  0x01
#define DEFER_DATA_MASK 0x06
#define DEFER_DATA_HI   0x02
#define DEFER_DATA_LO   0x04

/**
   Construct a client interface.
 */
Client::Client()
:m_state(State_Disabled)
,m_shifter(0)
,m_ack(false)
,m_bitcount(0)
,m_hold(false)
,m_cycle_manager(nullptr)
,m_deferred_drive(0)
{}

/**
   Initialisation of the interface, must be called before any operation.
 */
void Client::init(CycleManager& manager)
{
    m_cycle_manager = &manager;
}


void Client::set_state(State state)
{
    m_state = state;
    m_bitcount = 0;

    if (m_state == State_Disabled) {
        m_cycle_manager->cancel(*this);
        set_data_drive(true);
        set_clock_drive(true);
        m_hold = false;
    }
    else if (ClientStateFlags[state] & StateFlag_Hold) {
        set_clock_drive(false);
        m_hold = true;
    }

    m_signal.raise(Signal_StateChanged, (int) state);
}

/**
   Enable/disable the interface.
 */
void Client::set_enabled(bool enabled)
{
    if (m_state != State_Disabled && !enabled)
        set_state(State_Disabled);
    else if (m_state == State_Disabled && enabled && m_cycle_manager)
        set_state(State_Idle);
}

/**
   Reset the interface to the Idle state. No-op if the interface is disabled.
 */
void Client::reset()
{
    if (m_state == State_Disabled) return;

    m_hold = false;

    m_cycle_manager->cancel(*this);
    set_data_drive(true);
    set_clock_drive(true);

    if (ClientStateFlags[m_state] & StateFlag_Active)
        m_signal.raise(Signal_BusStateChanged, Bus_Idle);

    if (m_state != State_Idle)
        set_state(State_Idle);
}

/**
   Returns whether the interface is currently active, i.e. participating in bus traffic.
   For a client, it means it has positively acknowledged the address byte.
 */
bool Client::active() const
{
    return ClientStateFlags[m_state] & StateFlag_Active;
}

/**
   Returns whether the client is currently holding the clock line.
 */
bool Client::clock_hold() const
{
    return m_hold;
}

/**
   Returns the direction of the current request, depending on the latest RW bit received.
   \return 1 for a Read Request, 0 for a Write Request
 */
unsigned char Client::rw() const
{
    return (ClientStateFlags[m_state] & StateFlag_RW) ? 1 : 0;
}

/**
   Set the ack reply, after either an address or a data byte has been received.
   \param ack true for ACK, false for NACK
   \return true if the call was 'legal' i.e. the host was waiting for a ACK, false otherwise
 */
bool Client::set_ack(bool ack)
{
    if ((m_state == State_AddressRAck || m_state == State_AddressWAck || m_state == State_DataRxAck) && m_hold) {
        m_hold = false;
        m_ack = ack;
        set_data_drive(not ack);
        defer_clock_release();
        return true;
    } else {
        return false;
    }
}

/**
   Start transmitting a data byte, in response to a Read Request.
   \param data byte to be transmitted
   \return true if the call was 'legal', false otherwise
 */
bool Client::start_data_tx(uint8_t data)
{
    if (m_state == State_DataTx && m_hold) {
        m_hold = false;
        m_shifter = data;
        set_data_drive(data & 0x80);
        defer_clock_release();
        return true;
    } else {
        return false;
    }
}

/**
   Start receiving a data byte, in response to a Write Request.
   \return true if the call was 'legal', false otherwise
 */
bool Client::start_data_rx()
{
    if (m_state == State_DataRx && m_hold) {
        m_hold = false;
        m_shifter = 0x00;
        defer_clock_release();
        return true;
    } else {
        return false;
    }
}


void Client::clock_level_changed(bool level)
{
    switch (m_state) {

        case State_Start: {
            if (!level) {
                //Negative edge of the START bit, prepare to receive the address
                m_shifter = 0;
                set_state(State_AddressRx);
            }
        } break;

        case State_AddressRx: {
            //Receiving the address+RW
            //On positive edge, shift and sample
            if (level) {
                m_shifter = (m_shifter << 1) | (get_data_level() ? 0x01 : 0x00);
            }
            //On negative edge, count to 8 then raise the signal
            else if (m_bitcount < 7) {
                ++m_bitcount;
            }
            else {
                uint8_t addr_byte = m_shifter;
                set_state((addr_byte & 0x01) ? State_AddressRAck : State_AddressWAck);
                m_signal.raise(Signal_AddressReceived, addr_byte);
            }
        } break;

        case State_AddressRAck: {
            //On negative edge, after sending the ACK/NACK response to the address+R, change
            //state accordingly.
            if (!level) {
                if (m_ack) {
                    set_state(State_DataTx);
                    m_signal.raise(Signal_BusStateChanged, Bus_Busy);
                    m_signal.raise(Signal_DataStandby, 1U);
                } else {
                    set_state(State_Idle);
                }
            }
        } break;

        case State_AddressWAck: {
            //On negative edge, after sending the ACK/NACK response to the address+W, change
            //state accordingly.
            if (!level) {
                if (m_ack) {
                    m_shifter = 0;
                    defer_data_drive(true);
                    set_state(State_DataRx);
                    m_signal.raise(Signal_BusStateChanged, Bus_Busy);
                    m_signal.raise(Signal_DataStandby, 1U);
                } else {
                    set_state(State_Idle);
                }
            }
        } break;

        case State_DataTx: {
            //On positive edge, check for data collision.
            if (level) {
                if (get_data_level() && !get_data_drive()) {
                    defer_data_drive(true);
                    set_state(State_Idle);
                    m_signal.raise(Signal_BusCollision);
                    m_signal.raise(Signal_BusStateChanged, Bus_Idle);
                }
            //On negative edge, shift and write to the data line
            } else {
                m_shifter <<= 1;
                defer_data_drive(m_shifter & 0x80);
                if (m_bitcount < 7) {
                    ++m_bitcount;
                } else {
                    defer_data_drive(true);
                    set_state(State_DataTxAck);
                    m_signal.raise(Signal_DataSent);
                }
            }
        } break;

        case State_DataTxAck: {
            if (level) {
                m_ack = !get_data_level();
            } else {
                m_signal.raise(Signal_DataAckReceived, (unsigned int) m_ack);
                if (m_ack) {
                    set_state(State_DataTx);
                    m_signal.raise(Signal_DataStandby, 0U);
                } else {
                    set_state(State_Idle);
                }
            }
        } break;

        case State_DataRx: {
            if (level) {
                m_shifter = (m_shifter << 1) | (get_data_level() ? 1 : 0);
            }
            else if (m_bitcount < 7) {
                ++m_bitcount;
            }
            else {
                uint8_t data = m_shifter;
                set_state(State_DataRxAck);
                m_signal.raise(Signal_DataReceived, data);
            }
        } break;

        case State_DataRxAck: {
            if (!level) {
                defer_data_drive(true);
                m_signal.raise(Signal_DataAckSent);
                if (m_ack) {
                    set_state(State_DataRx);
                    m_signal.raise(Signal_DataStandby, 0U);
                } else {
                    set_state(State_Idle);
                }
            }
        } break;

        default: break;
    }
}


void Client::data_level_changed(bool level)
{
    if (m_state == State_Disabled || !get_clock_level()) return;

    bool was_active = active();

    if (level) {
        set_state(State_Idle);
        m_signal.raise(Signal_Stop, (unsigned int) was_active);
    } else {
        set_state(State_Start);
        m_signal.raise(Signal_Start, (unsigned int) was_active);
    }

    if (was_active)
        m_signal.raise(Signal_BusStateChanged, Bus_Idle);
}


void Client::defer_clock_release()
{
    m_deferred_drive |= DEFER_CLOCK_HI;
    if (!scheduled())
        m_cycle_manager->delay(*this, 1);
}


void Client::defer_data_drive(bool level)
{
    m_deferred_drive = (m_deferred_drive & ~DEFER_DATA_MASK) | (level ? DEFER_DATA_HI : DEFER_DATA_LO);
    if (!scheduled())
        m_cycle_manager->delay(*this, 1);
}


cycle_count_t Client::next(cycle_count_t when)
{
    if (m_deferred_drive & DEFER_DATA_MASK) {
        set_data_drive((m_deferred_drive & DEFER_DATA_MASK) == DEFER_DATA_HI);
        m_deferred_drive &= ~DEFER_DATA_MASK;
    } else if (m_deferred_drive & DEFER_CLOCK_HI) {
        set_clock_drive(true);
        m_deferred_drive &= ~DEFER_CLOCK_HI;
    }

    return m_deferred_drive ? 1 : 0;
}


//=======================================================================================

static const uint16_t HostStateFlags[Host::State_Count] {
    0x000, //Disabled :     no flag
    0x000, //Idle:          no flag
    0x067, //Start:         BusBusy,Active,Timer,Drive,ArbCheck
    0x0FF, //AddressTx:     BusBusy,Active,Timer,Hold,ByteShift,Drive,ArbCheck,ClockStretch
    0x087, //AddressAck:    BusBusy,Active,Timer,ClockStretch
    0x0FF, //DataTx:        BusBusy,Active,Timer,Hold,ByteShift,Drive,ArbCheck,ClockStretch
    0x087, //DataTxAck:     BusBusy,Active,Timer,ClockStretch
    0x09F, //DataRx:        BusBusy,Active,Timer,Hold,ByteShift,ClockStretch
    0x0AF, //DataRxAck:     BusBusy,Active,Timer,Hold,Drive,ClockStretch
    0x027, //Stop:          BusBusy,Active,Timer,Drive,ClockStretch
    0x001, //BusBusy:       BusBusy
    0x001, //ArbLost:       BusBusy
};


static const uint8_t PATTERN_START    = 0b00110001;
static const uint8_t PATTERN_RESTART  = 0b01100011;
static const uint8_t PATTERN_ZERO     = 0b01100000;
static const uint8_t PATTERN_ONE      = 0b01101111;
static const uint8_t PATTERN_STOP     = 0b11001000;

/**
   Construct a host interface
 */
Host::Host()
:m_state(State_Disabled)
,m_flags(HostStateFlags[State_Disabled])
,m_shifter(0)
,m_step(0)
,m_bitcount(0)
,m_addr_rw(0)
,m_ack(false)
,m_step_delay(1)
,m_pattern(0)
,m_cycle_manager(nullptr)
,m_hold(false)
{}

/**
   Initialisation of the interface, must be called before any operation.
 */
void Host::init(CycleManager& manager)
{
    m_cycle_manager = &manager;
}


void Host::set_state(State state)
{
    m_state = state;
    m_flags = HostStateFlags[state];
    m_step = 0;
    m_bitcount = 0;
    m_hold = m_flags & StateFlag_Hold;
    m_signal.raise(Signal_StateChanged, (int) state);
}

/**
   Enable/disable the interface.
 */
void Host::set_enabled(bool enabled)
{
    if (enabled) {
        if (m_state == State_Disabled && m_cycle_manager)
            set_state(State_Idle);
    } else {
        if (m_state != State_Disabled)
            set_state(State_Disabled);
    }
}

/**
   Reset the interface to the Idle state. No-op if the interface is disabled.
 */
void Host::reset()
{
    if (m_state != State_Disabled) {
        set_data_drive(true);
        set_clock_drive(true);
        m_cycle_manager->cancel(*this);
    }

    if (m_flags & StateFlag_Active)
        set_state(State_Idle);
}

/**
   Set the duration of one bit.
   \param delay bit duration in simulation cycles
 */
void Host::set_bit_delay(cycle_count_t delay)
{
    m_step_delay = delay / 4;
    if (m_step_delay < 1)
        m_step_delay = 1;
}

/**
   Returns whether the bus is currently busy.
 */
bool Host::bus_busy() const
{
    return m_flags & StateFlag_BusBusy;
}

/**
   Returns whether the host is currently active, i.e. participating in bus traffic.
   For a host, it means it's currently owning the bus.
 */
bool Host::active() const
{
    return m_flags & StateFlag_Active;
}

/**
   Returns whether the clock line is currently stretched by an end point other than this host.
 */
bool Host::clock_stretched() const
{
    return (m_flags & StateFlag_ClockStretch) && get_clock_drive() && !get_clock_level();
}


/**
   Start a transfer on the bus. A Start or Repeated Start condition will be transmitted.
   \return true if the call was 'legal', false otherwise
 */
bool Host::start_transfer()
{
    if (m_state == State_Idle ||
        ((m_state == State_DataTx || m_state == State_DataRx) && m_hold)) {

        m_pattern = (m_state == State_Idle) ? PATTERN_START : PATTERN_RESTART;
        set_state(State_Start);
        process_state_and_reschedule();
        return true;
    } else {
        return false;
    }
}

/**
   Send an address byte on the bus.
   \return true if the call was 'legal', false otherwise
 */
bool Host::set_address(uint8_t addr_rw)
{
    if (m_state == State_AddressTx && m_hold) {
        m_hold = false;
        m_shifter = m_addr_rw = addr_rw;
        process_state_and_reschedule();
        return true;
    } else {
        return false;
    }
}

/**
   Start transmitting a data byte, for a Write Request.
   \param data byte to be transmitted
   \return true if the call was 'legal', false otherwise
 */
bool Host::start_data_tx(uint8_t data)
{
    if (m_state == State_DataTx && m_hold) {
        m_hold = false;
        m_shifter = data;
        process_state_and_reschedule();
        return true;
    } else {
        return false;
    }
}

/**
   Start receiving a data byte, for a Read Request.
   \return true if the call was 'legal', false otherwise
 */
bool Host::start_data_rx()
{
    if (m_state == State_DataRx && m_hold) {
        m_hold = false;
        m_shifter = 0;
        process_state_and_reschedule();
        return true;
    } else {
        return false;
    }
}

/**
   End a transfer, a Stop condition will be transmitted.
   \return true if the call was 'legal', false otherwise
 */
bool Host::stop_transfer()
{
    if ((m_state == State_DataTx || m_state == State_DataRx) && m_hold) {
        set_state(State_Stop);
        m_pattern = PATTERN_STOP;
        process_state_and_reschedule();
        return true;
    } else {
        return false;
    }
}

/**
   Set the ack reply, after a data byte has been received.
   \param ack true for ACK, false for NACK
   \return true if the call was 'legal' i.e. the interface was waiting for a ACK, false otherwise
 */
bool Host::set_ack(bool ack)
{
    if (m_state == State_DataRxAck && m_hold) {
        m_hold = false;
        m_ack = ack;
        m_pattern = ack ? PATTERN_ZERO : PATTERN_ONE;
        process_state_and_reschedule();
        return true;
    } else {
        return false;
    }
}


void Host::apply_pattern()
{
    bool scl, sda;
    if (m_flags & StateFlag_Active) {
        scl = m_pattern & (1 << (4 + m_step));
        sda = m_pattern & (1 << m_step);
    } else {
        scl = sda = true;
    }

    set_clock_drive(scl);
    set_data_drive(sda);
}


cycle_count_t Host::next(cycle_count_t when)
{
    bool restart_timer = process_state(true);
    return restart_timer ? (when + m_step_delay) : 0;
}


void Host::process_state_and_reschedule()
{
    if (!scheduled()) {
        bool start_timer = process_state(false);
        if (start_timer)
            m_cycle_manager->delay(*this, m_step_delay);
    }
}


bool Host::process_state(bool inc_step)
{
    if (inc_step) {
        if (m_step == 3) {
            if ((m_flags & StateFlag_ByteShift) && m_bitcount < 7) {
                ++m_bitcount;
                m_step = 0;
            } else {
                transition_state();
            }
        } else {
            ++m_step;
        }
    }

    if (m_step == 0) {
        if (m_hold) {
            m_pattern = PATTERN_ZERO;
        }
        else if (m_flags & StateFlag_ByteShift) {
            if ((m_shifter & 0x80) || !(m_flags & StateFlag_Drive))
                m_pattern = PATTERN_ONE;
            else
                m_pattern = PATTERN_ZERO;

            m_shifter <<= 1;
        }
    }
    else if (m_step == 2) {
        if ((m_flags & StateFlag_ByteShift) && !(m_flags & StateFlag_Drive))
            m_shifter = (m_shifter & 0xFE) | (get_data_level() ? 1 : 0);
    }

    apply_pattern();

    if ((m_flags & StateFlag_ArbCheck) && get_clock_level() && get_data_drive() && !get_data_level()) {
        set_clock_drive(true);
        State old_state = m_state;
        set_state(State_ArbLost);
        m_signal.raise(Signal_ArbitrationLost, old_state);
        m_signal.raise(Signal_BusStateChanged, Bus_Busy);
        return false;
    }

    if ((m_flags & StateFlag_ClockStretch) && get_clock_drive() && !get_clock_level())
        return false;

    return (m_flags & StateFlag_Timer) && !m_hold;
}


void Host::transition_state()
{
    switch (m_state) {
        case State_Start: {
            m_signal.raise(Signal_BusStateChanged, Bus_Owned);
            set_state(State_AddressTx);
            m_signal.raise(Signal_AddressStandby);
        } break;

        case State_AddressTx: {
            set_state(State_AddressAck);
            m_pattern = PATTERN_ONE;
        } break;

        case State_AddressAck: {
            m_signal.raise(Signal_AddressSent, (unsigned int) m_ack);
            set_state((m_addr_rw & 0x01) ? State_DataRx: State_DataTx);
            m_shifter = 0x00;
            m_signal.raise(Signal_DataStandby, 1U);
        } break;

        case State_DataTx: {
            set_state(State_DataTxAck);
            m_pattern = PATTERN_ONE;
            m_signal.raise(Signal_DataSent);
        } break;

        case State_DataTxAck: {
            m_signal.raise(Signal_DataAckReceived, (unsigned int) m_ack);
            set_state(State_DataTx);
            m_signal.raise(Signal_DataStandby, 0U);
        } break;

        case State_DataRx: {
            uint8_t data = m_shifter;
            set_state(State_DataRxAck);
            m_signal.raise(Signal_DataReceived, data);
        } break;

        case State_DataRxAck: {
            m_signal.raise(Signal_DataAckSent);
            set_state(State_DataRx);
            m_signal.raise(Signal_DataStandby, 0U);
        } break;

        case State_Stop: {
            set_state(State_Idle);
            m_signal.raise(Signal_Stop);
        } break;

        default: break;
    }
}


void Host::clock_level_changed(bool level)
{
    if (!level) return;

    if (m_state == State_AddressAck || m_state == State_DataTxAck)
        m_ack = !get_data_level();

    if ((m_flags & StateFlag_ClockStretch) && !scheduled())
        process_state_and_reschedule();
}


void Host::data_level_changed(bool level)
{
    if (m_state == State_Disabled || !get_clock_level()) return;

    if (level) {
        //Stop condition
        if (m_state == State_BusBusy || m_state == State_ArbLost)
            set_state(State_Idle);
        m_signal.raise(Signal_BusStateChanged, Bus_Idle);
    } else {
        //Start condition
        if (m_state == State_Idle)
            set_state(State_BusBusy);
        m_signal.raise(Signal_BusStateChanged, Bus_Busy);
    }
}
