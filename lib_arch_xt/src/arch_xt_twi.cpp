/*
 * arch_xt_twi.cpp
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

#include "arch_xt_twi.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_device.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(TWI_t, reg))

#define REG_OFS(reg) \
    offsetof(TWI_t, reg)


//=======================================================================================

ArchXT_TWI::ArchXT_TWI(uint8_t num, const ArchXT_TWIConfig& config)
:Peripheral(AVR_IOCTL_TWI(0x30 + num))
,m_config(config)
,m_has_address(false)
,m_has_master_rx_data(false)
,m_has_slave_rx_data(false)
,m_intflag_master(false)
,m_intflag_slave(false)
{}

bool ArchXT_TWI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), TWI_SDASETUP_bm);
    add_ioreg(REG_ADDR(DUALCTRL), 0); //Dual ctrl not implemented
    //DBGCTRL not supported
    add_ioreg(REG_ADDR(MCTRLA), TWI_ENABLE_bm | TWI_WIEN_bm | TWI_RIEN_bm);
    add_ioreg(REG_ADDR(MCTRLB), TWI_MCMD_gm | TWI_ACKACT_bm | TWI_FLUSH_bm);
    add_ioreg(REG_ADDR(MSTATUS));
    add_ioreg(REG_ADDR(MSTATUS), TWI_RXACK_bm, true);
    add_ioreg(REG_ADDR(MBAUD));
    add_ioreg(REG_ADDR(MADDR));
    add_ioreg(REG_ADDR(MDATA));
    add_ioreg(REG_ADDR(SCTRLA), TWI_ENABLE_bm | TWI_PIEN_bm | TWI_APIEN_bm | TWI_DIEN_bm);
    add_ioreg(REG_ADDR(SCTRLB), TWI_MCMD_gm | TWI_ACKACT_bm);
    add_ioreg(REG_ADDR(SSTATUS), TWI_BUSERR_bm | TWI_COLL_bm | TWI_APIF_bm | TWI_DIF_bm);
    add_ioreg(REG_ADDR(SSTATUS), TWI_AP_bm | TWI_DIR_bm | TWI_RXACK_bm | TWI_CLKHOLD_bm, true);
    add_ioreg(REG_ADDR(SADDR));
    add_ioreg(REG_ADDR(SDATA));
    add_ioreg(REG_ADDR(SADDRMASK));

    status &= m_intflag_master.init(
        device,
        regbit_t(REG_ADDR(MCTRLA), 0, TWI_WIEN_bm | TWI_RIEN_bm),
        regbit_t(REG_ADDR(MSTATUS), 0, TWI_WIF_bm | TWI_RIF_bm),
        m_config.iv_master);

    status &= m_intflag_slave.init(
        device,
        regbit_t(REG_ADDR(SCTRLA), 0, TWI_PIEN_bm | TWI_APIEN_bm | TWI_DIEN_bm),
        regbit_t(REG_ADDR(SSTATUS), 0, TWI_APIF_bm | TWI_DIF_bm),
        m_config.iv_slave);

    m_twi.init(*device.cycle_manager(), logger());
    m_twi.signal().connect(*this);

    return status;
}

void ArchXT_TWI::reset()
{
    m_twi.reset();
    m_has_address = false;
    m_has_master_rx_data = false;
    m_has_slave_rx_data = false;
}

bool ArchXT_TWI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
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

uint8_t ArchXT_TWI::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(MDATA)) {
        //If data had been received previously, we need to send a ACK/NACK first.
        if (m_has_master_rx_data) {
            bool ack = (READ_IOREG(MCTRLB) & TWI_ACKACT_bm) == TWI_ACKACT_ACK_gc;
            m_twi.set_master_ack(ack);
            m_has_master_rx_data = false;
        }

        //Reading the register triggers a new read operation on the bus, if it's
        //legal to do so.
        if (m_twi.start_master_rx())
            clear_master_status();
    }
    else if (reg_ofs == REG_OFS(SDATA)) {
        //If data had been received previously, we need to send a ACK/NACK first.
        if (m_has_slave_rx_data) {
            bool ack = (READ_IOREG(SCTRLB) & TWI_ACKACT_bm) == TWI_ACKACT_ACK_gc;
            m_twi.set_slave_ack(ack);
            m_has_slave_rx_data = false;
        }

        //Reading the register triggers a new read operation on the bus, if it's
        //legal to do so.
        if (m_twi.start_slave_rx())
            clear_slave_status();
    }

    return value;
}

void ArchXT_TWI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //===============================================================
    //Master registers

    if (reg_ofs == REG_OFS(MCTRLA)) {
        //Update of the ENABLE bit
        if (data.posedge() & TWI_ENABLE_bm)
            set_master_enabled(true);
        else if (data.negedge() & TWI_ENABLE_bm)
            set_master_enabled(false);

        //Update of the WIEN or RIEN bits
        m_intflag_master.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(MCTRLB)) {
        //Flush of the TWI interface by disabling then enabling it
        if (data.value & TWI_FLUSH_bm) {
            if (TEST_IOREG(MCTRLA, TWI_ENABLE)) {
                set_master_enabled(false);
                set_master_enabled(true);
            }
            //FLUSH is a strobe bit so we clear it
            CLEAR_IOREG(MCTRLB, TWI_FLUSH);
            //Flushing takes over commands so no further processing other than clearing it
            WRITE_IOREG_F(MCTRLB, TWI_MCMD, 0);
            return;
        }

        uint8_t mcmd = data.value & TWI_MCMD_gm;
        //Acknowledgment Action, if MCMD is not zero and the master part is in the relevant state
        if (mcmd && m_twi.master_state() == TWI::State_RX_Ack) {
            bool ack = (data.value & TWI_ACKACT_bm) == TWI_ACKACT_ACK_gc;
            m_twi.set_master_ack(ack);
        }
        //The next step depends on MCMD
        switch(mcmd) {
            case TWI_MCMD_REPSTART_gc: { //Repeated Start condition
                uint8_t addr = READ_IOREG(MADDR);
                m_twi.send_address(addr >> 1, addr & 1);
            } break;

            case TWI_MCMD_RECVTRANS_gc: { //Next byte read operation, no-op for write operation
                if (READ_IOREG(MADDR) & 1)
                    m_twi.start_master_rx();
            } break;

            case TWI_MCMD_STOP_gc: { //Stop condition
                m_twi.end_transfer();
            } break;
        }

        //MCMD is a strobe so we clear it
        WRITE_IOREG_F(MCTRLB, TWI_MCMD, 0);
    }

    else if (reg_ofs == REG_OFS(MSTATUS)) {
        //Clears the write-one-to-clear status flags
        uint8_t flag_value = data.value & ~TWI_BUSSTATE_gm;
        WRITE_IOREG(MSTATUS, data.old & ~flag_value);

        //If writing IDLE to BUSSTATE, it resets the master part
        if ((data.value & TWI_BUSSTATE_gm) == TWI_BUSSTATE_IDLE_gc) {
            set_master_enabled(false);
            set_master_enabled(true);
        }

        m_intflag_master.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(MBAUD)) {
        cycle_count_t bitdelay = 10 + 2 * data.value; //Ignore T_rise effect for now
        m_twi.set_bit_delay(bitdelay);
    }

    else if (reg_ofs == REG_OFS(MADDR)) {
        //Clears all the master status flags
        clear_master_status();

        switch (READ_IOREG_F(MSTATUS, TWI_BUSSTATE)) {
            case TWI_BUSSTATE_UNKNOWN_gc:
                SET_IOREG(MSTATUS, TWI_BUSERR);
                m_intflag_master.set_flag(TWI_WIF_bm);
                break;

            case TWI_BUSSTATE_IDLE_gc: {
                bool acquired = m_twi.start_transfer();
                if (acquired) {
                    m_twi.send_address(data.value >> 1, data.value & 1);
                } else {
                    WRITE_IOREG_F(MSTATUS, TWI_BUSSTATE, TWI_BUSSTATE_BUSY_gc);
                    SET_IOREG(MSTATUS, TWI_ARBLOST);
                    m_intflag_master.set_flag(TWI_WIF_bm);
                }
                //Note that the BUSSTATE field is updated via the signal
            } break;

            case TWI_BUSSTATE_BUSY_gc:
                m_has_address = true;
                break;

            case TWI_BUSSTATE_OWNER_gc:
                m_twi.start_transfer();
                m_twi.send_address(data.value >> 1, data.value & 1);
                break;
        }
    }

    else if (reg_ofs == REG_OFS(MDATA)) {
        //State checks are done by the lower layer object. All we have left to do
        //is update the flags or restore the old register content if the
        //operation was illegal
        if (m_twi.start_master_tx(data.value))
            m_intflag_master.clear_flag();
        else
            WRITE_IOREG(MDATA, data.old);
    }

    //===============================================================
    //Slave registers

    else if (reg_ofs == REG_OFS(SCTRLA)) {
        //Update of the ENABLE bit
        if (data.posedge() & TWI_ENABLE_bm) {
            m_twi.set_slave_enabled(true);
        }
        else if (data.negedge() & TWI_ENABLE_bm) {
            m_twi.set_slave_enabled(false);
            m_has_slave_rx_data = false;
            clear_slave_status();
        }

        //Update of the PIEN, APIEN or DIEN bits
        m_intflag_slave.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(SSTATUS)) {
        //Clears the write-one-to-clear status flags, leaving unchanged the other bits
        const uint8_t flag_mask = TWI_DIF_bm | TWI_APIF_bm | TWI_COLL_bm | TWI_BUSERR_bm;
        uint8_t new_status = data.old & ~(data.value & flag_mask);
        WRITE_IOREG(SSTATUS, new_status);
        m_intflag_slave.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(SDATA)) {
        //State checks are done by the lower layer object. All we have left to do
        //is update the flags or restore the old register content if the
        //operation was illegal
        if (m_twi.start_slave_tx(data.value))
            clear_slave_status();
        else
            WRITE_IOREG(MDATA, data.old);
    }

}

void ArchXT_TWI::raised(const signal_data_t& sigdata, int)
{
    switch (sigdata.sigid) {

        case TWI::Signal_BusStateChange: {
            uint8_t bus_state = 0;
            switch(sigdata.data.as_int()) {
                case TWI::Bus_Idle: {
                    bus_state = TWI_BUSSTATE_IDLE_gc;
                    //If we had written an address, but the bus was busy,
                    //we can now start a transaction
                    if (m_has_address) {
                        m_has_address = false;
                        uint8_t addr = READ_IOREG(MADDR);
                        bool acquired = m_twi.start_transfer();
                        if (acquired) {
                            m_twi.send_address(addr >> 1, addr & 1);
                        } else {
                            SET_IOREG(MSTATUS, TWI_ARBLOST);
                            m_intflag_master.set_flag(TWI_WIF_bm);
                        }
                    }

                    //If the slave part is enabled, raise the Stop condition interrupt
                    if (TEST_IOREG(SCTRLA, TWI_ENABLE)) {
                        CLEAR_IOREG(SSTATUS, TWI_AP);
                        m_intflag_slave.set_flag(TWI_APIF_bm);
                    }

                } break;

                case TWI::Bus_Busy:
                    bus_state = TWI_BUSSTATE_BUSY_gc; break;

                case TWI::Bus_Owned:
                    bus_state = TWI_BUSSTATE_OWNER_gc; break;
            }

            if (TEST_IOREG(MCTRLA, TWI_ENABLE))
                WRITE_IOREG_F(MSTATUS, TWI_BUSSTATE, bus_state);

        } break;

        case TWI::Signal_Address: { //slave side only

            //Test the address with the match logic and set the ACK/NACK response
            uint8_t addr_rw = sigdata.data.as_uint();
            bool match = address_match(addr_rw);
            m_twi.set_slave_ack(match);
            //If it's a match, store the raw address byte in the data register
            //and update the status flags
            if (match) {
                WRITE_IOREG(SDATA, addr_rw);
                WRITE_IOREG_B(SSTATUS, TWI_DIR, addr_rw & 1);
                SET_IOREG(SSTATUS, TWI_AP);
                SET_IOREG(SSTATUS, TWI_CLKHOLD);
                m_intflag_slave.set_flag(TWI_APIF_bm);
            }

        } break;

        case TWI::Signal_AddrAck: { //Master side only

            if (sigdata.data.as_uint()) {
                //the address has been ACK'ed
                CLEAR_IOREG(MSTATUS, TWI_RXACK);
                //If it's a READ operation, continue by reading the first byte
                //If it's WRITE, hold the bus
                if (m_twi.master_state() & TWI::StateFlag_Tx)
                    SET_IOREG(MSTATUS, TWI_CLKHOLD);
                else
                    m_twi.start_master_rx();
            } else {
                //the address has been NACK'ed
                SET_IOREG(MSTATUS, TWI_RXACK);
                SET_IOREG(MSTATUS, TWI_CLKHOLD);
            }

            m_intflag_master.set_flag(TWI_WIF_bm);

        } break;

        case TWI::Signal_TxComplete: {

            if (sigdata.index == TWI::Cpt_Master) {

                //Update the status flags and raise the interrupt
                WRITE_IOREG_B(MSTATUS, TWI_RXACK, !sigdata.data.as_uint());
                SET_IOREG(MSTATUS, TWI_CLKHOLD);
                m_intflag_master.set_flag(TWI_WIF_bm);

            } else { //slave

                //Update the status flags and raise the interrupt
                WRITE_IOREG_B(SSTATUS, TWI_RXACK, !sigdata.data.as_uint());
                SET_IOREG(SSTATUS, TWI_CLKHOLD);
                m_intflag_slave.set_flag(TWI_DIF_bm);

            }

        } break;

        case TWI::Signal_RxComplete: {
            if (sigdata.index == TWI::Cpt_Master) {

                //Saves the received byte in the data register
                WRITE_IOREG(MDATA, sigdata.data.as_uint());
                m_has_master_rx_data = true;
                //Update the status flags and raise the interrupt
                SET_IOREG(MSTATUS, TWI_CLKHOLD);
                m_intflag_master.set_flag(TWI_RIF_bm);

            } else { //slave

                //Saves the received byte in the data register
                WRITE_IOREG(SDATA, sigdata.data.as_uint());
                m_has_slave_rx_data = true;
                //Update the status flags and raise the interrupt
                SET_IOREG(SSTATUS, TWI_CLKHOLD);
                m_intflag_slave.set_flag(TWI_DIF_bm);

            }
        } break;
    }
}

void ArchXT_TWI::set_master_enabled(bool enabled)
{
    if (enabled) {
        m_twi.set_master_enabled(true);
        WRITE_IOREG_F(MSTATUS, TWI_BUSSTATE, TWI_BUSSTATE_IDLE_gc);
    } else {
        m_twi.set_master_enabled(false);
        m_has_address = false;
        m_has_master_rx_data = false;
        clear_master_status();
        WRITE_IOREG_F(MSTATUS, TWI_BUSSTATE, TWI_BUSSTATE_UNKNOWN_gc);
    }
}

void ArchXT_TWI::clear_master_status()
{
    bitmask_t bm = bitmask_t(0, TWI_BUSERR_bm | TWI_ARBLOST_bm | TWI_CLKHOLD_bm);
    clear_ioreg(REG_ADDR(MSTATUS), bm);
    m_intflag_master.clear_flag();
}

void ArchXT_TWI::clear_slave_status()
{
    bitmask_t bm = bitmask_t(0, TWI_BUSERR_bm | TWI_COLL_bm | TWI_CLKHOLD_bm);
    clear_ioreg(REG_ADDR(SSTATUS), bm);
    m_intflag_slave.clear_flag();
}

bool ArchXT_TWI::address_match(uint8_t bus_address)
{
    //if PMEN is set, all addresses are recognized
    if (TEST_IOREG(SCTRLA, TWI_PMEN))
        return true;

    uint8_t reg_address = READ_IOREG(SADDR);
    bool gencall_enabled = reg_address & 1;
    reg_address >>= 1;

    //General call
    if (bus_address == 0x00 && gencall_enabled)
        return true;

    uint8_t addrmask = READ_IOREG(SADDRMASK);
    bool mask_enabled = addrmask & 0x01;

    if (mask_enabled) {
        addrmask >>= 1;
        return (bus_address | addrmask) == (reg_address | addrmask);
    } else {
        return bus_address == reg_address;
    }
}
