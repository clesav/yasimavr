/*
 * arch_avr_twi.cpp
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

#include "arch_avr_twi.h"
#include "core/sim_device.h"
#include <ioctrl_common/sim_twi.h>

YASIMAVR_USING_NAMESPACE

using namespace TWI;


//=======================================================================================

/****************************************************************************
  TWI State codes
****************************************************************************/
// General TWI Master status codes
#define TWI_START                  0x08  // START has been transmitted
#define TWI_REP_START              0x10  // Repeated START has been transmitted
#define TWI_ARB_LOST               0x38  // Arbitration lost

// TWI Master Transmitter status codes
#define TWI_MTX_ADR_ACK            0x18  // SLA+W has been transmitted and ACK received
#define TWI_MTX_ADR_NACK           0x20  // SLA+W has been transmitted and NACK received
#define TWI_MTX_DATA_ACK           0x28  // Data byte has been transmitted and ACK received
#define TWI_MTX_DATA_NACK          0x30  // Data byte has been transmitted and NACK received

// TWI Master Receiver status codes
#define TWI_MRX_ADR_ACK            0x40  // SLA+R has been transmitted and ACK received
#define TWI_MRX_ADR_NACK           0x48  // SLA+R has been transmitted and NACK received
#define TWI_MRX_DATA_ACK           0x50  // Data byte has been received and ACK transmitted
#define TWI_MRX_DATA_NACK          0x58  // Data byte has been received and NACK transmitted

// TWI Slave Transmitter status codes
#define TWI_STX_ADR_ACK            0xA8  // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0  // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK           0xB8  // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK          0xC0  // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = 0); ACK has been received

// TWI Slave Receiver status codes
#define TWI_SRX_ADR_ACK            0x60  // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK            0x70  // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78  // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK       0x80  // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK      0x88  // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK       0x90  // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK      0x98  // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART       0xA0  // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE               0xF8  // No relevant state information available
#define TWI_BUS_ERROR              0x00  // Bus error due to an illegal START or STOP condition


//=======================================================================================

class ArchAVR_TWI::_PinDriver : public PinDriver {

public:

    _PinDriver(ArchAVR_TWI& per);

    void set_enabled(bool enable);
    void set_host_mode(bool is_host);

    void set_host_line_state(TWI::Line line, bool state);
    void set_client_line_state(TWI::Line line, bool state);

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool dig_state) override;

private:

    ArchAVR_TWI& m_peripheral;
    bool m_enabled;
    bool m_is_host;
    bool m_line_states[2];

};


//=======================================================================================

class ArchAVR_TWI::_Host : public Host {

public:

    _Host(ArchAVR_TWI& per) : m_peripheral(per) {}

protected:

    virtual void set_line_state(TWI::Line line, bool dig_state) override
    {
        m_peripheral.m_driver->set_host_line_state(line, dig_state);
    }

private:

    ArchAVR_TWI& m_peripheral;

};


//=======================================================================================

class ArchAVR_TWI::_Client : public Client {

public:

    _Client(ArchAVR_TWI& per) : m_peripheral(per) {}

protected:

    virtual void set_line_state(TWI::Line line, bool dig_state) override
    {
        m_peripheral.m_driver->set_client_line_state(line, dig_state);
    }

private:

    ArchAVR_TWI& m_peripheral;

};


//=======================================================================================

ArchAVR_TWI::_PinDriver::_PinDriver(ArchAVR_TWI& per)
:PinDriver(per.id(), 2)
,m_peripheral(per)
,m_enabled(false)
,m_is_host(false)
,m_line_states{true, true}
{}


void ArchAVR_TWI::_PinDriver::set_enabled(bool enable)
{
    PinDriver::set_enabled(TWI::Line_Clock, enable);
    PinDriver::set_enabled(TWI::Line_Data, enable);
}


void ArchAVR_TWI::_PinDriver::set_host_mode(bool is_host)
{
    m_is_host = is_host;
    if (is_host) {
        m_line_states[TWI::Line_Clock] = m_peripheral.m_host->get_clock_drive();
        m_line_states[TWI::Line_Data] = m_peripheral.m_host->get_data_drive();
    } else {
        m_line_states[TWI::Line_Clock] = m_peripheral.m_client->get_clock_drive();
        m_line_states[TWI::Line_Data] = m_peripheral.m_client->get_data_drive();
    }

    update_pin_state(TWI::Line_Clock);
    update_pin_state(TWI::Line_Data);
}


void ArchAVR_TWI::_PinDriver::set_host_line_state(TWI::Line line, bool state)
{
    if (m_is_host) {
        m_line_states[line] = state;
        update_pin_state(line);
    }
}


void ArchAVR_TWI::_PinDriver::set_client_line_state(TWI::Line line, bool state)
{
    if (!m_is_host) {
        m_line_states[line] = state;
        update_pin_state(line);
    }
}


Pin::controls_t ArchAVR_TWI::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = {
        .dir = (unsigned char) (!m_line_states[pin_index]),
        .drive = 0,
        .pull_up = true,
    };
    return c;
}


void ArchAVR_TWI::_PinDriver::digital_state_changed(pin_index_t pin_index, bool dig_state)
{
    m_peripheral.m_host->line_state_changed((TWI::Line) pin_index, dig_state);
    m_peripheral.m_client->line_state_changed((TWI::Line) pin_index, dig_state);
}


//=======================================================================================

ArchAVR_TWI::ArchAVR_TWI(uint8_t num, const ArchAVR_TWIConfig& config)
:Peripheral(AVR_IOCTL_TWI(0x30 + num))
,m_config(config)
,m_host_hook(*this, &ArchAVR_TWI::host_signal_raised)
,m_client_hook(*this, &ArchAVR_TWI::client_signal_raised)
,m_gencall(false)
,m_latched_ack(false)
,m_intflag(false)
{
    m_driver = new _PinDriver(*this);
    m_host = new _Host(*this);
    m_client = new _Client(*this);
}


ArchAVR_TWI::~ArchAVR_TWI()
{
    delete m_driver;
    delete m_host;
    delete m_client;
}


bool ArchAVR_TWI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(regbit_t(m_config.reg_ctrl, m_config.bm_enable));
    add_ioreg(regbit_t(m_config.reg_ctrl, m_config.bm_start));
    add_ioreg(regbit_t(m_config.reg_ctrl, m_config.bm_stop));
    add_ioreg(regbit_t(m_config.reg_ctrl, m_config.bm_int_enable));
    add_ioreg(regbit_t(m_config.reg_ctrl, m_config.bm_int_flag));
    add_ioreg(regbit_t(m_config.reg_ctrl, m_config.bm_ack_enable));
    add_ioreg(m_config.reg_bitrate);
    add_ioreg(m_config.rb_status, true);
    add_ioreg(m_config.rb_prescaler);
    add_ioreg(m_config.reg_data);
    add_ioreg(m_config.rb_addr);
    add_ioreg(m_config.rb_gencall_enable);
    add_ioreg(m_config.rb_addr_mask);

    status &= m_intflag.init(device,
                             regbit_t(m_config.reg_ctrl, m_config.bm_int_enable),
                             regbit_t(m_config.reg_ctrl, m_config.bm_int_flag),
                             m_config.iv_twi);

    m_host->init(*device.cycle_manager());
    m_client->init(*device.cycle_manager());

    m_host->signal().connect(m_host_hook);
    m_client->signal().connect(m_client_hook);

    status &= device.pin_manager().register_driver(*m_driver);

    return status;
}


void ArchAVR_TWI::reset()
{
    Peripheral::reset();
    m_host->set_enabled(false);
    m_client->set_enabled(false);
    m_driver->set_enabled(false);
    clear_flag_and_status();
    m_gencall = false;
    write_ioreg(m_config.reg_data, 0xFF);
    write_ioreg(m_config.rb_addr, 0x7F);
}


bool ArchAVR_TWI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = (data->index == 0) ? &m_host->signal() : &m_client->signal();
        return true;
    }
    return false;
}


void ArchAVR_TWI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    logger().dbg("Writing register 0x%02x -> [0x%02x]", data.value, addr);

    if (addr == m_config.reg_ctrl) {
        //TWEN
        bool enabled = m_config.bm_enable.extract(data.value);
        m_host->set_enabled(enabled);
        m_client->set_enabled(enabled);
        m_driver->set_enabled(enabled);

        //TWIE
        m_intflag.update_from_ioreg();

        //TWINT is a write-one-to-clear and for the rest of processing, we need
        //to know if it's cleared.
        bool intflag_cleared;
        if (m_config.bm_int_flag.extract(data.value)) {
            clear_flag_and_status();
            intflag_cleared = true;
        } else {
            intflag_cleared = !m_config.bm_int_flag.extract(data.old);
        }

        if (enabled && intflag_cleared) {
            m_latched_ack = m_config.bm_ack_enable.extract(data.value);
            bool sta = m_config.bm_start.extract(data.value);
            bool sto = m_config.bm_stop.extract(data.value);
            execute_command(sta, sto);
        }

        clear_ioreg(m_config.reg_ctrl, m_config.bm_stop);
    }

    //TWBR and TWPS
    if (addr == m_config.rb_prescaler.addr || addr == m_config.reg_bitrate) {
        uint8_t ps_index = read_ioreg(m_config.rb_prescaler);
        unsigned long ps_factor = m_config.ps_factors[ps_index];
        uint8_t bitrate = read_ioreg(m_config.reg_bitrate);
        m_host->set_bit_delay(16 + 2 * bitrate * ps_factor);
    }

    if (addr == m_config.reg_data) {
        //The data register is writable only when the interrupt flag is set
        if (!test_ioreg(m_config.reg_ctrl, m_config.bm_int_flag))
            write_ioreg(m_config.reg_data, data.old);
    }
}


void ArchAVR_TWI::host_signal_raised(const signal_data_t& sigdata, int)
{
    switch (sigdata.sigid) {
        case TWI::Signal_AddressStandby:
            raise_flag_and_status(TWI_START);
            break;

        case TWI::Signal_DataStandby: {
            uint8_t status;
            if (sigdata.data.as_uint()) {
                if (m_host->rw())
                    status = m_host->ack() ? TWI_MRX_ADR_ACK : TWI_MRX_ADR_NACK;
                else
                    status = m_host->ack() ? TWI_MTX_ADR_ACK : TWI_MTX_ADR_NACK;
                logger().dbg("Address sent, received %s", m_host->ack() ? "ACK" : "NACK");
            } else {
                if (m_host->rw()) {
                    status = m_host->ack() ? TWI_MRX_DATA_ACK : TWI_MRX_DATA_NACK;
                } else {
                    status = m_host->ack() ? TWI_MTX_DATA_ACK : TWI_MTX_DATA_NACK;
                    logger().dbg("Host data sent, received %s", m_host->ack() ? "ACK" : "NACK");
                }

            }
            raise_flag_and_status(status);
        } break;

        case TWI::Signal_DataReceived: {
            write_ioreg(m_config.reg_data, (uint8_t) sigdata.data.as_uint());
            bool acken = test_ioreg(m_config.reg_ctrl, m_config.bm_ack_enable);
            m_host->set_ack(acken);
            logger().dbg("Data received 0x%02x, replying with %s", sigdata.data.as_uint(), acken ? "ACK" : "NACK");
        } break;

        case TWI::Signal_ArbitrationLost:
            logger().dbg("Arbitration lost");
            m_driver->set_host_mode(false);
            raise_flag_and_status(TWI_ARB_LOST);
            break;

        case TWI::Signal_Stop:
            m_driver->set_host_mode(false);
            break;

        default: break;
    }
}


void ArchAVR_TWI::client_signal_raised(const signal_data_t& sigdata, int)
{
    switch (sigdata.sigid) {
        case TWI::Signal_AddressReceived: {
            uint8_t addr_rw = sigdata.data.as_uint();
            bool match = address_match(addr_rw);
            m_client->set_ack(match);
        } break;

        case TWI::Signal_DataStandby: {
            if (sigdata.data.as_uint()) {
                //For the first byte we need to signal that we've matched the address
                //The status also depends whether we arrived here after an arbitration loss
                //and if it was a General Call address.
                uint8_t status;
                bool arblost = m_host->state() == Host::State_ArbLost;
                if (m_client->rw())
                    status = arblost ? TWI_STX_ADR_ACK_M_ARB_LOST : TWI_STX_ADR_ACK;
                else if (m_gencall)
                    status = arblost ? TWI_SRX_GEN_ACK_M_ARB_LOST : TWI_SRX_GEN_ACK;
                else
                    status = arblost ? TWI_SRX_ADR_ACK_M_ARB_LOST : TWI_SRX_ADR_ACK;
                raise_flag_and_status(status);
            }
        } break;

        case TWI::Signal_DataReceived: {
            //Save the received data in the register and set the ACK bit reply
            logger().dbg("Client data received: 0x%02x", sigdata.data.as_uint());
            write_ioreg(m_config.reg_data, sigdata.data.as_uint());
            m_client->set_ack(m_latched_ack);
        } break;

        case TWI::Signal_DataAckSent: {
            //On completion of transmitting the ACK after receiving data, set the status
            logger().dbg("Client data ACK bit sent");
            uint8_t status;
            if (m_gencall)
                status = m_latched_ack ? TWI_SRX_GEN_DATA_ACK : TWI_SRX_GEN_DATA_NACK;
            else
                status = m_latched_ack ? TWI_SRX_ADR_DATA_ACK : TWI_SRX_ADR_DATA_NACK;
            raise_flag_and_status(status);
        } break;

        case TWI::Signal_DataAckReceived: {
            //On receiving a ACK bit after transmitting data, set the status
            logger().dbg("Client data sent, received %s", sigdata.data.as_uint() ? "ACK" : "NACK");
            uint8_t status;
            if (!sigdata.data.as_uint())
                status = TWI_STX_DATA_NACK;
            else if (m_latched_ack)
                status = TWI_STX_DATA_ACK;
            else
                status= TWI_STX_DATA_ACK_LAST_BYTE;
            raise_flag_and_status(status);
        } break;

        case TWI::Signal_Start:
        case TWI::Signal_Stop: {
            if (sigdata.data.as_uint())
                raise_flag_and_status(TWI_SRX_STOP_RESTART);
        } break;
    }
}


void ArchAVR_TWI::execute_command(bool sta, bool sto)
{
    //if TWSTO=1
    if (sto) {
        m_host->stop_transfer();
        m_client->reset();
        m_driver->set_host_mode(false);
    }
    //if TWSTA=1 and TWSTO=0
    else if (sta) {
        if (m_host->start_transfer())
            m_driver->set_host_mode(true);
    }
    //if TWSTA=0 and TWSTO=0, in host mode and the interface is expecting user input
    else if (m_host->active() && m_host->clock_hold()) {
        switch (m_host->state()) {

            case Host::State_AddressTx: {
                uint8_t addr_rw = read_ioreg(m_config.reg_data);
                logger().dbg("Setting address 0x%02x", addr_rw);
                m_host->set_address(addr_rw);
            } break;

            case Host::State_DataTx: {
                uint8_t data = read_ioreg(m_config.reg_data);
                logger().dbg("Setting host TX data 0x%02x", data);
                m_host->start_data_tx(data);
            } break;

            case Host::State_DataRx: {
                logger().dbg("Starting host RX data");
                m_host->start_data_rx();
            } break;

            default: break;

        }
    }
    //if TWSTA=0 and TWSTO=0, in client mode and the interface is expecting user input
    else if (m_client->active() && m_client->clock_hold()) {
        switch (m_client->state()) {

            case Client::State_DataTx: {
                uint8_t data = read_ioreg(m_config.reg_data);
                logger().dbg("Setting client TX data 0x%02x", data);
                m_client->start_data_tx(data);
            } break;

            case Client::State_DataRx: {
                logger().dbg("Starting client RX data");
                m_client->start_data_rx();
            } break;

            default: break;

        }
    }
}


void ArchAVR_TWI::raise_flag_and_status(uint8_t status)
{
    if (read_ioreg(m_config.rb_status) == TWI_NO_STATE >> 3)
        write_ioreg(m_config.rb_status, status >> 3);
    m_intflag.set_flag();
}


void ArchAVR_TWI::clear_flag_and_status()
{
    write_ioreg(m_config.rb_status, TWI_NO_STATE >> 3);
    m_intflag.clear_flag();
}


bool ArchAVR_TWI::address_match(uint8_t addr_rw)
{
    if (!test_ioreg(m_config.reg_ctrl, m_config.bm_ack_enable))
        return false;

    //Check the general call
    m_gencall = (addr_rw == 0x00) && test_ioreg(m_config.rb_gencall_enable);
    if (m_gencall)
        return true;

    uint8_t dev_addr = read_ioreg(m_config.rb_addr);
    uint8_t mask = read_ioreg(m_config.rb_addr_mask);
    return (dev_addr | mask) == ((addr_rw >> 1) | mask);
}
