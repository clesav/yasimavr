/*
 * arch_avr_twi.cpp
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

#include "arch_avr_twi.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


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
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = �0�); ACK has been received

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
#define TWI_NO_STATE               0xF8  // No relevant state information available; TWINT = �0�
#define TWI_BUS_ERROR              0x00  // Bus error due to an illegal START or STOP condition



//=======================================================================================

ArchAVR_TWI::ArchAVR_TWI(uint8_t num, const ArchAVR_TWIConfig& config)
:Peripheral(AVR_IOCTL_TWI(0x30 + num))
,m_config(config)
,m_gencall(false)
,m_rx(false)
,m_intflag(false)
{}

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

    m_twi.init(*device.cycle_manager(), logger());
    m_twi.signal().connect(*this);

    return status;
}

void ArchAVR_TWI::reset()
{
    m_twi.reset();
    clear_intflag();
    write_ioreg(m_config.reg_data, 0xFF);
    write_ioreg(m_config.rb_addr, 0x7F);
}

bool ArchAVR_TWI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_twi.signal();
        return true;
    }
    else if (req == AVR_CTLREQ_TWI_ENDPOINT) {
        data->data = &m_twi;
        return true;
    }

    return false;
}

void ArchAVR_TWI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    if (addr == m_config.reg_ctrl) {
        //TWEN
        bool enabled = m_config.bm_enable.extract(data.value);
        //If enabled does not change, these are nop.
        m_twi.set_master_enabled(enabled);
        m_twi.set_slave_enabled(enabled);

        //TWIE
        m_intflag.update_from_ioreg();

        //TWINT is a write-one-to-clear and for the rest of processing, we need
        //to know if it's cleared.
        bool intflag_clear;
        if (m_config.bm_int_flag.extract(data.value)) {
            clear_intflag();
            intflag_clear = true;
        } else {
            intflag_clear = !m_config.bm_int_flag.extract(data.old);
        }

        if (enabled && intflag_clear) {
            //if TWSTO=1
            if (m_config.bm_stop.extract(data.value)) {
                m_twi.end_transfer();
                m_twi.set_slave_enabled(false);
                m_twi.set_slave_enabled(true);
            }
            //if TWSTA=1 and TWSTO=0
            else if (m_config.bm_start.extract(data.value)) {
                if (m_twi.start_transfer())
                    set_intflag(TWI_START);
            }
            //if TWSTA=0 and TWSTO=0
            else {
                switch (m_twi.master_state()) {
                    case TWI::State_Waiting: {
                        //Clearing TWSTA when queueing for bus ownership resets
                        //the arbitration logic
                        m_twi.set_master_enabled(false);
                        m_twi.set_master_enabled(true);
                    } break;

                    case TWI::State_Addr: {
                        //send address+RW, they are stored in the Data register
                        uint8_t sla = read_ioreg(m_config.reg_data);
                        m_rx = sla & 1;
                        m_twi.send_address(sla >> 1, m_rx);
                    } break;

                    case TWI::State_TX: {
                        uint8_t tx_data = read_ioreg(m_config.reg_data);
                        m_twi.start_master_tx(tx_data);
                    } break;

                    case TWI::State_RX: {
                        m_twi.start_master_rx();
                    } break;

                    default: break;
                }
            }
        }

        clear_ioreg(m_config.reg_ctrl, m_config.bm_stop);
    }

    //TWBR and TWPS
    if (addr == m_config.rb_prescaler.addr || addr == m_config.reg_bitrate) {
        uint8_t ps_index = read_ioreg(m_config.rb_prescaler);
        unsigned long ps_factor = m_config.ps_factors[ps_index];
        uint8_t bitrate = read_ioreg(m_config.reg_bitrate);
        m_twi.set_bit_delay(16 + 2 * bitrate * ps_factor);
    }

    if (addr == m_config.reg_data) {
        //The data register is writable only when either the master or the slave part
        //is active and not busy.
        //In any other case, we must restore the previous register content.
        TWI::State ms = m_twi.master_state();
        TWI::State ss = m_twi.slave_state();
        if ((!(ms & TWI::StateFlag_Active) || (ms & TWI::StateFlag_Busy)) &&
            (!(ss & TWI::StateFlag_Active) || (ss & TWI::StateFlag_Busy)))
            write_ioreg(m_config.reg_data, data.old);
    }

}

void ArchAVR_TWI::raised(const signal_data_t& sigdata, int)
{
    uint8_t ctrl = read_ioreg(m_config.reg_ctrl);

    bool start = m_config.bm_start.extract(ctrl);

    switch (sigdata.sigid) {

        case TWI::Signal_BusStateChange: {
            if (sigdata.data.as_int() == TWI::Bus_Idle) {
                if (start) {
                    if (m_twi.start_transfer())
                        set_intflag(TWI_START);
                }

                if (m_twi.slave_state() & TWI::StateFlag_Active)
                    set_intflag(TWI_SRX_STOP_RESTART);

            }
        } break;

        case TWI::Signal_Address: { //slave side only
            //The data register stores the byte received (address + rw)
            uint8_t addr_rw = sigdata.data.as_uint();
            write_ioreg(m_config.reg_data, addr_rw);
            //Test the address with the match logic and set the ACK/NACK response
            if (test_ioreg(m_config.reg_ctrl, m_config.bm_ack_enable)) {
                //Case of a general call (ADDR=0x00), only accepted if it's a Write and gencall
                //is enabled
                if (!(addr_rw >> 1)) {
                    if (!(addr_rw & 1) && test_ioreg(m_config.rb_gencall_enable)) {
                        m_twi.set_slave_ack(true);
                        m_gencall = true;
                        m_rx = true;
                        set_intflag(TWI_SRX_GEN_ACK);
                    }
                }
                //Other addresses, use the match logic
                else if (address_match(addr_rw >> 1)) {
                    m_twi.set_slave_ack(true);
                    m_gencall = false;
                    m_rx = !(addr_rw & 1);
                    set_intflag(m_rx ? TWI_SRX_ADR_ACK : TWI_STX_ADR_ACK);
                }
                //Otherwise, reply with NACK
                else {
                    m_twi.set_slave_ack(false);
                }
            } else {
                m_twi.set_slave_ack(false);
            }

        } break;

        case TWI::Signal_AddrAck: { //Master side only

            uint8_t status;
            if (sigdata.data.as_int() == TWIPacket::Ack)
                status = m_rx ? TWI_MRX_ADR_ACK : TWI_MTX_ADR_ACK;
            else
                status = m_rx ? TWI_MRX_ADR_NACK : TWI_MTX_ADR_NACK;

            set_intflag(status);

        } break;

        case TWI::Signal_TxComplete: {

            bool acken = test_ioreg(m_config.reg_ctrl, m_config.bm_ack_enable);
            uint8_t status;
            if (sigdata.index == TWI::Cpt_Master) //master side
                status = (sigdata.data.as_uint() == TWIPacket::Ack) ? TWI_MTX_DATA_ACK : TWI_MTX_DATA_NACK;
            else { //slave
                if (sigdata.data.as_int() == TWIPacket::Nack)
                    status = TWI_STX_DATA_NACK;
                else if (acken)
                    status = TWI_STX_DATA_ACK;
                else
                    status = TWI_STX_DATA_ACK_LAST_BYTE;
            }

            set_intflag(status);

        } break;

        case TWI::Signal_RxComplete: {

            //Save the received byte in the data register
            write_ioreg(m_config.reg_data, sigdata.data.as_uint());
            //Set the status and reply automatically with ACK or NACK depending on TWIEA
            bool acken = test_ioreg(m_config.reg_ctrl, m_config.bm_ack_enable);
            uint8_t status;
            if (sigdata.index == TWI::Cpt_Master) {
                m_twi.set_master_ack(acken);
                if (acken)
                    status = TWI_MRX_DATA_ACK;
                else
                    status = TWI_MRX_DATA_NACK;
            } else { //slave part
                m_twi.set_slave_ack(acken);
                if (m_gencall)
                    status = acken ? TWI_SRX_GEN_DATA_ACK : TWI_SRX_GEN_DATA_NACK;
                else
                    status = acken ? TWI_SRX_ADR_DATA_ACK : TWI_SRX_ADR_DATA_NACK;
            }

            set_intflag(status);

        } break;
    }
}

void ArchAVR_TWI::set_intflag(uint8_t status)
{
    write_ioreg(m_config.rb_status, status >> 3);
    m_intflag.set_flag();
}

void ArchAVR_TWI::clear_intflag()
{
    write_ioreg(m_config.rb_status, TWI_NO_STATE >> 3);
    m_intflag.clear_flag();
}

bool ArchAVR_TWI::address_match(uint8_t bus_address)
{
    uint8_t reg_address = read_ioreg(m_config.rb_addr);
    uint8_t addrmask = read_ioreg(m_config.rb_addr_mask);
    return (bus_address | addrmask) == (reg_address | addrmask);
}
