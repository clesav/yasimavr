/*
 * arch_xt_twi.cpp
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

#include "arch_xt_twi.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"
#include "ioctrl_common/sim_twi.h"

YASIMAVR_USING_NAMESPACE

using namespace TWI;

//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(TWI_t, reg))

#define REG_OFS(reg) \
    offsetof(TWI_t, reg)


enum HookTag {
    Tag_Host,
    Tag_Client
};


static const cycle_count_t BaseBaud = 10;


//=======================================================================================

class ArchXT_TWI::_PinDriver : public PinDriver {

public:

    explicit _PinDriver(ArchXT_TWI& per, bool dual_enabled);

    void set_enabled(bool enabled);
    void set_dual_mode(bool dual);

    void set_host_driver_state(Line line, bool dig_state);
    void set_client_driver_state(Line line, bool dig_state);

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool dig_state) override;

private:

    enum DualMode {
        Dual_Disabled,
        Dual_Off,
        Dual_On,
    };

    ArchXT_TWI& m_peripheral;
    bool m_enabled;
    DualMode m_dual;
    bool m_line_states[4];

};


class ArchXT_TWI::_Host : public Host {

public:

    _Host(ArchXT_TWI& per) : m_peripheral(per) {}

protected:

    virtual void set_line_state(Line line, bool dig_state) override
    {
        m_peripheral.m_driver->set_host_driver_state(line, dig_state);
    }

private:

    ArchXT_TWI& m_peripheral;

};


class ArchXT_TWI::_Client : public Client {

public:

    _Client(ArchXT_TWI& per) : m_peripheral(per) {}

protected:

    virtual void set_line_state(Line line, bool dig_state) override
    {
        m_peripheral.m_driver->set_client_driver_state(line, dig_state);
    }

private:

    ArchXT_TWI& m_peripheral;

};


//=======================================================================================

ArchXT_TWI::_PinDriver::_PinDriver(ArchXT_TWI& per, bool dual_enabled)
:PinDriver(per.id(), dual_enabled ? 4 : 2)
,m_peripheral(per)
,m_enabled(false)
,m_dual(dual_enabled ? Dual_Off : Dual_Disabled)
,m_line_states{true, true, true, true}
{}


void ArchXT_TWI::_PinDriver::set_enabled(bool enabled)
{
    m_enabled = enabled;
    PinDriver::set_enabled(Line_Clock, enabled);
    PinDriver::set_enabled(Line_Data, enabled);
    if (m_dual != Dual_Disabled) {
        bool dual = m_dual == Dual_On;
        PinDriver::set_enabled(Line_Clock + 2, enabled && dual);
        PinDriver::set_enabled(Line_Data + 2, enabled && dual);
    }
}


void ArchXT_TWI::_PinDriver::set_dual_mode(bool dual)
{
    if (m_dual == Dual_Disabled) return;
    m_dual = dual ? Dual_On : Dual_Off;
    update_pin_state(Line_Clock);
    update_pin_state(Line_Data);
    PinDriver::set_enabled(Line_Clock + 2, m_enabled && dual);
    PinDriver::set_enabled(Line_Data + 2, m_enabled && dual);
}


void ArchXT_TWI::_PinDriver::set_host_driver_state(Line line, bool state)
{
    m_line_states[line] = state;
    update_pin_state(line);
}


void ArchXT_TWI::_PinDriver::set_client_driver_state(Line line, bool state)
{
    m_line_states[line + 2] = state;
    update_pin_state((m_dual == Dual_On) ? (line + 2) : line);
}


Pin::controls_t ArchXT_TWI::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = { .drive = 0, .pull_up = true };
    if (m_dual == Dual_On)
        c.dir = !m_line_states[pin_index];
    else
        c.dir = !m_line_states[pin_index] || !m_line_states[pin_index + 2];

    return c;
}


void ArchXT_TWI::_PinDriver::digital_state_changed(pin_index_t pin_index, bool dig_state)
{
    if (pin_index < 2) {
        m_peripheral.m_host->line_state_changed((Line) pin_index, dig_state);
        if (m_dual != Dual_On)
            m_peripheral.m_client->line_state_changed((Line) pin_index, dig_state);
    } else {
        m_peripheral.m_client->line_state_changed((Line)(pin_index - 2), dig_state);
    }
}


//=======================================================================================

ArchXT_TWI::ArchXT_TWI(uint8_t num, const ArchXT_TWIConfig& config)
:Peripheral(AVR_IOCTL_TWI(0x30 + num))
,m_config(config)
,m_host_hook(*this, &ArchXT_TWI::host_signal_raised)
,m_client_hook(*this, &ArchXT_TWI::client_signal_raised)
,m_pending_host_address(false)
,m_pending_host_rx_data(false)
,m_pending_client_rx_data(false)
,m_host_cmd(0)
,m_client_cmd(0)
,m_intflag_host(false)
,m_intflag_client(false)
{
    m_client = new _Client(*this);
    m_host = new _Host(*this);
    m_driver = new _PinDriver(*this, m_config.dual_ctrl);
}


ArchXT_TWI::~ArchXT_TWI()
{
    delete m_client;
    delete m_host;
    delete m_driver;
}


bool ArchXT_TWI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), TWI_FMPEN_bm | TWI_SDAHOLD_gm | TWI_SDASETUP_bm);
    if (m_config.dual_ctrl)
        add_ioreg(REG_ADDR(DUALCTRL), TWI_ENABLE_bm | TWI_FMPEN_bm | TWI_SDAHOLD_gm);
    //DBGCTRL not supported
    add_ioreg(REG_ADDR(MCTRLA), TWI_ENABLE_bm | TWI_SMEN_bm | TWI_WIEN_bm | TWI_RIEN_bm);
    add_ioreg(REG_ADDR(MCTRLB), TWI_MCMD_gm | TWI_ACKACT_bm | TWI_FLUSH_bm);
    add_ioreg(REG_ADDR(MSTATUS), TWI_BUSSTATE_gm | TWI_BUSERR_bm | TWI_ARBLOST_bm | TWI_CLKHOLD_bm | TWI_WIF_bm | TWI_RIF_bm);
    add_ioreg(REG_ADDR(MSTATUS), TWI_RXACK_bm, true);
    add_ioreg(REG_ADDR(MBAUD));
    add_ioreg(REG_ADDR(MADDR));
    add_ioreg(REG_ADDR(MDATA));
    add_ioreg(REG_ADDR(SCTRLA), TWI_ENABLE_bm | TWI_SMEN_bm | TWI_PMEN_bm | TWI_PIEN_bm | TWI_APIEN_bm | TWI_DIEN_bm);
    add_ioreg(REG_ADDR(SCTRLB), TWI_SCMD_gm | TWI_ACKACT_bm);
    add_ioreg(REG_ADDR(SSTATUS), TWI_BUSERR_bm | TWI_COLL_bm | TWI_APIF_bm | TWI_DIF_bm);
    add_ioreg(REG_ADDR(SSTATUS), TWI_AP_bm | TWI_DIR_bm | TWI_RXACK_bm | TWI_CLKHOLD_bm, true);
    add_ioreg(REG_ADDR(SADDR));
    add_ioreg(REG_ADDR(SDATA));
    add_ioreg(REG_ADDR(SADDRMASK));

    status &= m_intflag_host.init(
        device,
        regbit_t(REG_ADDR(MCTRLA), 0, TWI_WIEN_bm | TWI_RIEN_bm),
        regbit_t(REG_ADDR(MSTATUS), 0, TWI_WIF_bm | TWI_RIF_bm),
        m_config.iv_host);

    status &= m_intflag_client.init(
        device,
        regbit_t(REG_ADDR(SCTRLA), 0, TWI_APIEN_bm | TWI_DIEN_bm),
        regbit_t(REG_ADDR(SSTATUS), 0, TWI_APIF_bm | TWI_DIF_bm),
        m_config.iv_client);

    m_host->init(*device.cycle_manager());
    m_host->signal().connect(m_host_hook);

    m_client->init(*device.cycle_manager());
    m_client->signal().connect(m_client_hook);

    status &= device.pin_manager().register_driver(*m_driver);

    return status;
}


void ArchXT_TWI::reset()
{
    m_host->set_enabled(false);
    m_host->set_bit_delay(BaseBaud);
    m_client->set_enabled(false);
    m_driver->set_enabled(false);
    m_pending_host_address = false;
    m_pending_host_rx_data = false;
    m_pending_client_rx_data = false;
    m_host_cmd = 0;
    m_client_cmd = 0;
}


bool ArchXT_TWI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = (data->index == 0) ? &m_host->signal() : &m_client->signal();
        return true;
    }
    else if (req == AVR_CTLREQ_TWI_BUS_ERROR) {
        if (data->index == 0 && m_host->enabled()) {
            reset_host();
            SET_IOREG(MSTATUS, TWI_BUSERR);
            SET_IOREG(MSTATUS, TWI_ARBLOST);
            m_intflag_host.set_flag(TWI_WIF_bm);
        }

        if ((data->index != 0 || m_host->enabled()) && m_client->enabled()) {
            m_client->reset();
            SET_IOREG(SSTATUS, TWI_BUSERR);
        }

        return true;
    }
    return false;
}


uint8_t ArchXT_TWI::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;
    logger().dbg("Reading register [0x%02x] -> 0x%02x", reg_ofs, value);

    if (reg_ofs == REG_OFS(MDATA)) {
        //If data had been received previously, clear the status
        if (m_pending_host_rx_data) {
            clear_host_status();
            //If the smart mode is enabled, send the acknowledgment bit.
            if (TEST_IOREG(MCTRLA, TWI_SMEN)) {
                bool ack = !TEST_IOREG(MCTRLB, TWI_ACKACT);
                m_host->set_ack(ack);
            }
        }
    }
    else if (reg_ofs == REG_OFS(SDATA)) {
        //If data had been received previously, clear the status
        if (m_pending_client_rx_data) {
            clear_client_status();
            if (TEST_IOREG(SCTRLA, TWI_SMEN)) {
                bool ack = !TEST_IOREG(MCTRLB, TWI_ACKACT);
                m_client->set_ack(ack);
            }
        }
    }

    return value;
}


uint8_t ArchXT_TWI::ioreg_peek_handler(reg_addr_t addr, uint8_t value)
{
    //Avoid triggering any action when peeking a register
    return value;
}


void ArchXT_TWI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;
    logger().dbg("Writing register 0x%02x -> [0x%02x]", data.value, reg_ofs);

    if (reg_ofs == REG_OFS(DUALCTRL)) {
        //Update of the dual mode, if supported
        if (m_config.dual_ctrl)
            m_driver->set_dual_mode(EXTRACT_B(data.value, TWI_ENABLE));
    }

    //===============================================================
    //Host registers

    else if (reg_ofs == REG_OFS(MCTRLA)) {
        //Update of the ENABLE bit
        if (data.posedge() & TWI_ENABLE_bm) {
            m_host->set_enabled(true);
        }
        else if (data.negedge() & TWI_ENABLE_bm) {
            reset_host();
            m_host->set_enabled(false);
        }

        m_driver->set_enabled(m_host->enabled() || m_client->enabled());

        //Update of the WIEN or RIEN bits
        m_intflag_host.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(MCTRLB)) {
        //Flush of the TWI interface by disabling then enabling it
        if (data.value & TWI_FLUSH_bm) {
            logger().dbg("Flush");
            if (TEST_IOREG(MCTRLA, TWI_ENABLE))
                reset_host();
            //FLUSH is a strobe bit so we clear it
            CLEAR_IOREG(MCTRLB, TWI_FLUSH);
            //Flushing takes over commands so no further processing other than clearing it
            WRITE_IOREG_GC(MCTRLB, TWI_MCMD, 0);
            return;
        }

        //Extract and clear MCMD, it's a strobe
        m_host_cmd = EXTRACT_GC(data.value, TWI_MCMD);
        WRITE_IOREG_GC(MCTRLB, TWI_MCMD, 0);

        //Acknowledgment Action, if MCMD is not zero and the host part is in the relevant state
        if (TEST_IOREG(MCTRLA, TWI_ENABLE) && m_host_cmd) {
            clear_host_status();
            bool ack = !EXTRACT_B(data.value, TWI_ACKACT);
            if (!m_host->set_ack(ack))
                execute_host_command();
        }
    }

    else if (reg_ofs == REG_OFS(MSTATUS)) {
        //Clears the write-one-to-clear status flags
        const uint8_t wotc_flags = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm;
        WRITE_IOREG(MSTATUS, data.old & ~(data.value & wotc_flags));

        //If writing IDLE to BUSSTATE, it resets the host part
        if (TEST_IOREG(MCTRLA, TWI_ENABLE) && EXTRACT_GC(data.value, TWI_BUSSTATE) == TWI_BUSSTATE_IDLE_gc) {
            reset_host();
            WRITE_IOREG_GC(MSTATUS, TWI_BUSSTATE, TWI_BUSSTATE_IDLE_gc);
        }

        m_intflag_host.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(MBAUD)) {
        cycle_count_t bitdelay = BaseBaud + 2 * data.value; //Ignore T_rise effect
        m_host->set_bit_delay(bitdelay);
    }

    else if (reg_ofs == REG_OFS(MADDR)) {
        if (!TEST_IOREG(MCTRLA, TWI_ENABLE)) {
            //Peripheral disabled, nothing to do
        }

        else if (READ_IOREG_GC(MSTATUS, TWI_BUSSTATE) == TWI_BUSSTATE_UNKNOWN_gc) {
            //If the bus state is unknown we can't do anything, just set the bus error flag
            SET_IOREG(MSTATUS, TWI_BUSERR);
            m_intflag_host.set_flag(TWI_WIF_bm);
        }

        else if (m_host->state() == Host::State_AddressTx) {
            //If the host is expecting an address
            logger().dbg("Sending address byte: 0x%02x", data.value);
            m_host->set_address(data.value);
            clear_host_status();
        }

        else if (m_host->start_transfer()) {
            //Else try to issue a start condition. The state checks are done by the lower layer object.
            //If it succeeds, the address will follow automatically.
            logger().dbg("Sending start condition");
            m_pending_host_address = true;
            WRITE_IOREG_GC(MSTATUS, TWI_BUSSTATE, TWI_BUSSTATE_OWNER_gc);
            clear_host_status();
        }
    }

    else if (reg_ofs == REG_OFS(MDATA)) {
        //State checks are done by the lower layer object. All we have left to do
        //is update the flags or restore the old register content if the
        //operation was illegal.
        if (m_host->start_data_tx(data.value))
            clear_host_status();
        else
            WRITE_IOREG(MDATA, data.old);
    }

    //===============================================================
    //Client registers

    else if (reg_ofs == REG_OFS(SCTRLA)) {
        //Update of the ENABLE bit
        if (data.posedge() & TWI_ENABLE_bm) {
            m_client->set_enabled(true);
        }
        else if (data.negedge() & TWI_ENABLE_bm) {
            m_client->set_enabled(false);
            clear_client_status();
        }

        m_driver->set_enabled(m_host->enabled() || m_client->enabled());

        //Update of the PIEN, APIEN or DIEN bits
        m_intflag_client.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(SCTRLB)) {
        //Extract and clear SCMD, it's a strobe
        uint8_t scmd = EXTRACT_GC(data.value, TWI_SCMD);
        WRITE_IOREG_GC(SCTRLB, TWI_SCMD, 0);

        //Command execution, if the client is enabled and SCMD is not zero
        if (TEST_IOREG(SCTRLA, TWI_ENABLE) && scmd) {
            logger().dbg("Client command: %d", scmd);
            m_client_cmd = scmd;
            clear_client_status();
            bool ack = !EXTRACT_B(data.value, TWI_ACKACT);
            if (!m_client->set_ack(ack))
                execute_client_command();
        }
    }

    else if (reg_ofs == REG_OFS(SSTATUS)) {
        //Clears the write-one-to-clear status flags
        const uint8_t wotc_flags = TWI_DIF_bm | TWI_APIF_bm | TWI_COLL_bm | TWI_BUSERR_bm;
        WRITE_IOREG(SSTATUS, data.old & ~(data.value & wotc_flags));
        m_intflag_client.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(SDATA)) {
        //State checks are done by the lower layer object. All we have left to do
        //is update the flags or restore the old register content if the
        //operation was illegal
        if (TEST_IOREG(SCTRLA, TWI_SMEN) && m_client->start_data_tx(data.value))
            clear_client_status();
        else if (m_client->state() != Client::State_DataTx)
            WRITE_IOREG(SDATA, data.old);
    }

}


static const TWI_BUSSTATE_t BusEnumToRegField[] = {
    TWI_BUSSTATE_IDLE_gc,
    TWI_BUSSTATE_BUSY_gc,
    TWI_BUSSTATE_OWNER_gc
};


void ArchXT_TWI::host_signal_raised(const signal_data_t& sigdata, int)
{
    switch (sigdata.sigid) {

        case Signal_BusStateChanged: {
            //If an address had been written but the bus was busy,
            //we can now start a transaction
            if (sigdata.data.as_int() == Bus_Idle && m_pending_host_address)
                m_host->start_transfer();

            //Update the BUSSTATE field
            TWI_BUSSTATE_t s = BusEnumToRegField[sigdata.data.as_int()];
            WRITE_IOREG_GC(MSTATUS, TWI_BUSSTATE, s);
        } break;

        case Signal_AddressStandby: {
            if (m_pending_host_address) {
                uint8_t addr_rw = READ_IOREG(MADDR);
                logger().dbg("Sending address 0x%02x after queuing", addr_rw);
                m_pending_host_address = false;
                m_host->set_address(addr_rw);
            }
        } break;

        case Signal_AddressSent: {
            //Save the ACK bit received
            bool ack = sigdata.data.as_uint();
            WRITE_IOREG_B(MSTATUS, TWI_RXACK, ack ? 0 : 1);
            logger().dbg("Address sent, received %s", ack ? "ACK" : "NACK");
        } break;

        case Signal_DataStandby: {
            //If it's a Read Request
            if (m_host->rw()) {
                //For the first byte, just continue with the byte transfer
                //For the follow-on bytes, execute the command we have one
                if (sigdata.data.as_uint())
                    m_host->start_data_rx();
                else
                    execute_host_command();
            } else {
                //For a Write Request, hold on until data input
                SET_IOREG(MSTATUS, TWI_CLKHOLD);
                m_intflag_host.set_flag(TWI_WIF_bm);
            }
        } break;

        case Signal_DataAckReceived: {
            //Update the status flags
            bool ack = sigdata.data.as_uint();
            logger().dbg("Host data sent, received %s", ack ? "ACK" : "NACK");
            WRITE_IOREG_B(MSTATUS, TWI_RXACK, ack ? 0 : 1);
        } break;

        case Signal_DataReceived: {
            //Save the received byte
            WRITE_IOREG(MDATA, sigdata.data.as_uint());
            m_pending_host_rx_data = true;
            //Update the status flags and raise the interrupt
            SET_IOREG(MSTATUS, TWI_CLKHOLD);
            m_intflag_host.set_flag(TWI_RIF_bm);
            logger().dbg("Host received data 0x%02x", sigdata.data.as_uint());
        } break;

        case Signal_ArbitrationLost: {
            SET_IOREG(MSTATUS, TWI_ARBLOST);
            m_intflag_host.set_flag(TWI_WIF_bm);
            m_pending_host_address = false;
        } break;

    }
}


void ArchXT_TWI::client_signal_raised(const signal_data_t& sigdata, int)
{
    switch (sigdata.sigid) {

        case Signal_AddressReceived: {
            //Test the address with the match logic. If it's a match,
            // store the raw address byte in the data register and update the status flags
            uint8_t addr_rw = sigdata.data.as_uint();
            logger().dbg("Client received address 0x%02x", addr_rw);
            if (address_match(addr_rw)) {
                WRITE_IOREG(SDATA, addr_rw);
                WRITE_IOREG_B(SSTATUS, TWI_DIR, addr_rw & 1);
                SET_IOREG(SSTATUS, TWI_AP);
                SET_IOREG(SSTATUS, TWI_CLKHOLD);
                m_intflag_client.set_flag(TWI_APIF_bm);
            }
        } break;

        case Signal_DataStandby: {
            //If it's a Read Request
            if (m_client->rw()) {
                //hold on until data input, set the status flags
                SET_IOREG(SSTATUS, TWI_CLKHOLD);
                m_intflag_client.set_flag(TWI_DIF_bm);
            } else {
                //For a Write Request, continue with the byte transfer
                m_client->start_data_rx();
            }
        } break;

        case Signal_DataAckReceived: {
            //A byte has been sent and ACK/NACK received from the host,
            //Save the ACK bit in the status register
            bool ack = sigdata.data.as_uint();
            WRITE_IOREG_B(SSTATUS, TWI_RXACK, ack ? 0 : 1);
            m_intflag_client.set_flag(TWI_DIF_bm);
            logger().dbg("Client sent data, received %s", ack ? "ACK" : "NACK");
        } break;

        case Signal_DataReceived: {
            //A byte has been received.
            WRITE_IOREG(SDATA, sigdata.data.as_uint());
            m_pending_client_rx_data = true;
            SET_IOREG(SSTATUS, TWI_CLKHOLD);
            m_intflag_client.set_flag(TWI_DIF_bm);
            logger().dbg("Client received data 0x%02x", sigdata.data.as_uint());
        } break;

        case Signal_BusCollision: {
            SET_IOREG(SSTATUS, TWI_COLL);
            m_intflag_client.set_flag(TWI_DIF_bm);
            logger().dbg("Client bus collision detected");
        } break;

        case Signal_Stop: {
            logger().dbg("Client detected a STOP condition");
            //Raise the Stop condition interrupt
            if (TEST_IOREG(SCTRLA, TWI_PIEN)) {
                CLEAR_IOREG(SSTATUS, TWI_AP);
                m_intflag_client.set_flag(TWI_APIF_bm);
            }
        } break;
    }
}


void ArchXT_TWI::reset_host()
{
    m_host->set_enabled(false);
    m_host->set_enabled(true);
    m_pending_host_address = false;
    clear_host_status();
    WRITE_IOREG_GC(MSTATUS, TWI_BUSSTATE, TWI_BUSSTATE_UNKNOWN_gc);
}


void ArchXT_TWI::clear_host_status()
{
    bitmask_t bm = bitmask_t(0, TWI_RIF_bm | TWI_WIF_bm | TWI_BUSERR_bm | TWI_ARBLOST_bm | TWI_CLKHOLD_bm);
    clear_ioreg(REG_ADDR(MSTATUS), bm);
    m_intflag_host.update_from_ioreg();
    m_pending_host_rx_data = false;
}


void ArchXT_TWI::clear_client_status()
{
    bitmask_t bm = bitmask_t(0, TWI_BUSERR_bm | TWI_COLL_bm | TWI_CLKHOLD_bm);
    clear_ioreg(REG_ADDR(SSTATUS), bm);
    m_intflag_client.clear_flag();
    m_pending_client_rx_data = false;
}


bool ArchXT_TWI::address_match(uint8_t addr_byte)
{
    //if PMEN is set, all addresses are recognized
    if (TEST_IOREG(SCTRLA, TWI_PMEN))
        return true;

    uint8_t reg_addr = READ_IOREG(SADDR);
    bool gen_call_enabled = reg_addr & 1;
    reg_addr >>= 1;

    //General call
    if (addr_byte == 0x00 && gen_call_enabled)
        return true;

    uint8_t rx_addr = addr_byte >> 1;
    uint8_t addr_mask = READ_IOREG_F(SADDRMASK, TWI_ADDRMASK);
    if (TEST_IOREG(SADDRMASK, TWI_ADDREN))
        return (rx_addr == reg_addr) || (rx_addr == addr_mask);
    else
        return (rx_addr | addr_mask) == (reg_addr | addr_mask);
}


void ArchXT_TWI::execute_host_command()
{
    logger().dbg("Executing host command: %d", m_host_cmd);

    switch(m_host_cmd) {
        case TWI_MCMD_REPSTART_gc: //Repeated Start condition
            m_host->start_transfer();
            break;

        case TWI_MCMD_RECVTRANS_gc: //Next byte read operation, no-op for write operation
            m_host->start_data_rx();
            break;

        case TWI_MCMD_STOP_gc: //Stop condition
            m_host->stop_transfer();
            break;
    }

    m_host_cmd = 0;
}


void ArchXT_TWI::execute_client_command()
{
    logger().dbg("Executing client command: %d", m_client_cmd);

    switch(m_client_cmd) {
        case TWI_SCMD_COMPTRANS_gc: //Complete Transaction
            m_client->reset();
            break;

        case TWI_SCMD_RESPONSE_gc: //Next byte write or read operation
            if (m_client->rw())
                m_client->start_data_tx(READ_IOREG(SDATA));
            else
                m_client->start_data_rx();
            break;
    }

    m_client_cmd = 0;
}
