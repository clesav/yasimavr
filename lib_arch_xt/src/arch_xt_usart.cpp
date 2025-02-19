/*
 * arch_xt_usart.cpp
 *
 *  Copyright 2022-2025 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_xt_usart.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"
#include "core/sim_sleep.h"
#include "core/sim_device.h"
#include <ioctrl_common/sim_uart.h>

YASIMAVR_USING_NAMESPACE

using namespace UART;

//=======================================================================================

class ArchXT_USART::_PinDriver : public PinDriver {

public:

    explicit _PinDriver(ArchXT_USART& per);

    void set_open_drain_enabled(bool enabled);
    void set_loopback_enabled(bool enabled);

    void set_gen_enabled(bool enable);
    void set_line_enabled(Line line, bool enable);

    void set_line_state(Line line, bool state);
    bool get_line_state(Line line) const;

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool dig_state) override;

private:

    ArchXT_USART& m_peripheral;
    bool m_gen_enabled;
    bool m_line_enabled[4];
    bool m_line_states[4];
    bool m_open_drain;
    bool m_loopback;

};


class ArchXT_USART::_Controller : public USART {

public:

    _Controller(ArchXT_USART& per) : m_peripheral(per) {}

protected:

    virtual void set_line_state(Line line, bool state) override
    {
        m_peripheral.m_driver->set_line_state(line, state);
    }

    virtual bool get_line_state(Line line) const override
    {
        return m_peripheral.m_driver->get_line_state(line);
    }

private:

    ArchXT_USART& m_peripheral;

};


//=======================================================================================

ArchXT_USART::_PinDriver::_PinDriver(ArchXT_USART& per)
:PinDriver(per.id(), 4)
,m_peripheral(per)
,m_gen_enabled(false)
,m_line_enabled{false, false, false, false}
,m_line_states{true, true, false, false}
,m_open_drain(false)
,m_loopback(false)
{}


void ArchXT_USART::_PinDriver::set_open_drain_enabled(bool enabled)
{
    m_open_drain = enabled;
    update_pin_state(Line_TXD);
}


void ArchXT_USART::_PinDriver::set_loopback_enabled(bool enabled)
{
    m_loopback = enabled;

    bool old_rxd_state = m_line_states[Line_RXD];
    bool new_rxd_state;
    if (m_loopback)
        new_rxd_state = m_line_states[Line_TXD];
    else
        new_rxd_state = pin_state(Line_TXD).digital_value();

    m_line_states[Line_RXD] = new_rxd_state;

    if (new_rxd_state != old_rxd_state)
        m_peripheral.m_ctrl->line_state_changed(Line_RXD, new_rxd_state);
}


void ArchXT_USART::_PinDriver::set_gen_enabled(bool enable)
{
    m_gen_enabled = enable;
    for (pin_index_t i = 0; i < 4; ++i)
        PinDriver::set_enabled(i, m_line_enabled[i] && enable);
}


void ArchXT_USART::_PinDriver::set_line_enabled(Line line, bool enable)
{
    m_line_enabled[line] = enable;
    PinDriver::set_enabled(line, enable && m_gen_enabled);
}


void ArchXT_USART::_PinDriver::set_line_state(Line line, bool state)
{
    m_line_states[line] = state;
    update_pin_state(line);
}


bool ArchXT_USART::_PinDriver::get_line_state(Line line) const
{
    return m_line_states[line];
}


Pin::controls_t ArchXT_USART::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = gpio_controls;
    switch ((Line) pin_index) {
        case Line_TXD: {
            c.dir = !m_line_states[Line_TXD] || !m_open_drain;
            c.drive = m_line_states[Line_TXD];
        } break;

        case Line_XCK: {
            c.drive = m_line_states[Line_XCK];
        } break;

        case Line_DIR: {
            c.drive = m_line_states[Line_DIR];
        } break;

        case Line_RXD: break;
    }

    return c;
}


void ArchXT_USART::_PinDriver::digital_state_changed(pin_index_t pin_index, bool dig_state)
{
    m_line_states[pin_index] = dig_state;

    if (pin_index != Line_RXD || !m_loopback)
        m_peripheral.m_ctrl->line_state_changed((Line) pin_index, dig_state);

    if (pin_index == Line_TXD && m_loopback) {
        m_line_states[Line_RXD] = dig_state;
        m_peripheral.m_ctrl->line_state_changed(Line_RXD, dig_state);
    }
}


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(USART_t, reg))

#define REG_OFS(reg) \
    offsetof(USART_t, reg)


ArchXT_USART::ArchXT_USART(uint8_t num, const ArchXT_USARTConfig& config)
:Peripheral(AVR_IOCTL_UART(0x30 + num))
,m_config(config)
,m_ctrl_hook(*this, &ArchXT_USART::ctrl_signal_raised)
,m_rxc_intflag(false)
,m_txc_intflag(false)
,m_txe_intflag(false)
{
    m_driver = new _PinDriver(*this);
    m_ctrl = new _Controller(*this);
}


ArchXT_USART::~ArchXT_USART()
{
    delete m_driver;
    delete m_ctrl;
}


bool ArchXT_USART::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(RXDATAL), USART_DATA_gm, true);
    add_ioreg(REG_ADDR(RXDATAH), USART_DATA8_bm | USART_PERR_bm | USART_FERR_bm | USART_BUFOVF_bm | USART_RXCIF_bm, true);
    add_ioreg(REG_ADDR(TXDATAL), USART_DATA_gm);
    add_ioreg(REG_ADDR(TXDATAH), USART_DATA8_bm);
    add_ioreg(REG_ADDR(STATUS), USART_RXCIF_bm | USART_DREIF_bm, true); // R/O part
    add_ioreg(REG_ADDR(STATUS), USART_TXCIF_bm | USART_RXSIF_bm); // R/W part
    add_ioreg(REG_ADDR(CTRLA), USART_RXCIE_bm | USART_TXCIE_bm | USART_DREIE_bm, USART_RXSIE_bm | USART_LBME_bm | USART_RS485_gm);
    add_ioreg(REG_ADDR(CTRLB), USART_RXEN_bm | USART_TXEN_bm | USART_SFDEN_bm | USART_ODME_bm | USART_RXMODE_gm);
    add_ioreg(REG_ADDR(CTRLC), USART_CMODE_gm | USART_PMODE_gm | USART_SBMODE_bm | USART_CHSIZE_gm);
    add_ioreg(REG_ADDR(BAUDL));
    add_ioreg(REG_ADDR(BAUDH));
    //CTRLD not implemented
    //DBGCTRL not supported
    //EVCTRL not implemented
    //TXPLCTRL not implemented
    //RXPLCTRL not implemented

    status &= m_rxc_intflag.init(device,
                                 regbit_t(REG_ADDR(CTRLA), 0, USART_RXCIE_bm | USART_RXSIE_bm),
                                 regbit_t(REG_ADDR(STATUS), 0, USART_RXCIF_bm | USART_RXSIF_bm),
                                 m_config.iv_rxc);
    status &= m_txc_intflag.init(device,
                                 DEF_REGBIT_B(CTRLA, USART_TXCIE),
                                 DEF_REGBIT_B(STATUS, USART_TXCIF),
                                 m_config.iv_txc);
    status &= m_txe_intflag.init(device,
                                 DEF_REGBIT_B(CTRLA, USART_DREIE),
                                 DEF_REGBIT_B(STATUS, USART_DREIF),
                                 m_config.iv_txe);

    m_ctrl->init(*device.cycle_manager(), &logger());
    m_ctrl->set_tx_buffer_limit(2);
    m_ctrl->set_rx_buffer_limit(3);
    m_ctrl->signal().connect(m_ctrl_hook);

    device.pin_manager().register_driver(*m_driver);

    return status;
}


void ArchXT_USART::reset()
{
    Peripheral::reset();

    m_driver->set_gen_enabled(false);
    m_driver->set_open_drain_enabled(false);
    m_driver->set_loopback_enabled(false);

    m_ctrl->reset();
    WRITE_IOREG_F(CTRLC, USART_CHSIZE, 0x03);
    SET_IOREG(STATUS, USART_DREIF);

    update_bitrate();
}


bool ArchXT_USART::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_ctrl->signal();
        return true;
    }
    else if (req == AVR_CTLREQ_USART_BYTES) {
        const uint8_t* s = data->data.as_bytes();
        for (size_t i = 0; i < data->data.size(); ++i)
            m_ctrl->push_rx_frame(s[i]);
        return true;
    }

    return false;
}


uint8_t ArchXT_USART::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(RXDATAL)) {
        if (READ_IOREG_GC(CTRLC, USART_CHSIZE) != USART_CHSIZE_9BITL_gc) {
            //Pop the value being read and shift the RX FIFO for the next read
            m_ctrl->pop_rx();
            extract_rx_data();
            CLEAR_IOREG(RXDATAH, USART_BUFOVF);
        }
    }

    else if (reg_ofs == REG_OFS(RXDATAH)) {
        if (READ_IOREG_GC(CTRLC, USART_CHSIZE) == USART_CHSIZE_9BITL_gc) {
            //Pop the value being read and shift the RX FIFO for the next read
            m_ctrl->pop_rx();
            extract_rx_data();
            CLEAR_IOREG(RXDATAH, USART_BUFOVF);
        }
    }

    return value;
}


uint8_t ArchXT_USART::ioreg_peek_handler(reg_addr_t addr, uint8_t value)
{
    //Avoid triggering an action by peeking RXDATA
    return value;
}


void ArchXT_USART::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    //Writing to TXDATA emits the value, if TX is enabled
    if (reg_ofs == REG_OFS(TXDATAL)) {
        if (TEST_IOREG(CTRLB, USART_TXEN) && READ_IOREG_GC(CTRLC, USART_CHSIZE) != USART_CHSIZE_9BITL_gc) {
            uint16_t frame = (READ_IOREG_B(TXDATAH, USART_DATA8) << 8) | data.value;
            m_ctrl->push_tx(frame);
            if (m_ctrl->tx_pending())
                m_txe_intflag.clear_flag();

            logger().dbg("Data pushed: 0x%03x", frame);
        }
    }

    else if (reg_ofs == REG_OFS(TXDATAH)) {
        if (TEST_IOREG(CTRLB, USART_TXEN) && READ_IOREG_GC(CTRLC, USART_CHSIZE) == USART_CHSIZE_9BITL_gc) {
            uint16_t frame = (EXTRACT_B(data.value, USART_DATA8) << 8) | READ_IOREG(TXDATAL);
            m_ctrl->push_tx(frame);
            if (m_ctrl->tx_pending())
                m_txe_intflag.clear_flag();

            logger().dbg("Data pushed: 0x%03x", frame);
        }
    }

    else if (reg_ofs == REG_OFS(STATUS)) {
        //Writing one to RXSIF clears the bit and cancels the interrupt
        if (data.value & USART_RXSIF_bm)
            m_rxc_intflag.clear_flag(USART_RXSIF_bm);
        //Writing one to TXCIF clears the bit and cancels the interrupt
        if (data.value & USART_TXCIF_bm)
            m_txc_intflag.clear_flag();
    }

    else if (reg_ofs == REG_OFS(CTRLA)) {
        m_txc_intflag.update_from_ioreg();
        m_txe_intflag.update_from_ioreg();
        m_rxc_intflag.update_from_ioreg();

        m_driver->set_loopback_enabled(data.value & USART_LBME_bm);

        //Determine if the DIR line is used, it's RS485[0].
        bool dir_line_enabled = EXTRACT_F(data.value, USART_RS485) & 1;
        m_driver->set_line_enabled(Line_DIR, dir_line_enabled);
        m_ctrl->set_tx_dir_enabled(dir_line_enabled);
    }

    else if (reg_ofs == REG_OFS(CTRLB)) {
        //Processing of TXEN
        if (data.posedge() & USART_TXEN_bm) {
            m_driver->set_line_enabled(Line_TXD, true);
        }
        //If TXEN is being cleared, flush the TX buffer
        else if (data.negedge() & USART_TXEN_bm) {
            m_ctrl->cancel_tx_pending();
            m_txe_intflag.set_flag();
            if (!m_ctrl->tx_in_progress())
                m_driver->set_line_enabled(Line_TXD, false);
        }

        //Processing of RXEN changes
        if (data.posedge() & USART_RXEN_bm) {
            m_ctrl->set_rx_enabled(true);
            m_driver->set_line_enabled(Line_RXD, true);
        }
        else if (data.negedge() & USART_RXEN_bm) {
            m_ctrl->set_rx_enabled(false);
            m_driver->set_line_enabled(Line_RXD, false);
            m_rxc_intflag.clear_flag(USART_RXCIF_bm);
        }

        //Open drain mode enable
        m_driver->set_open_drain_enabled(data.value & USART_ODME_bm);

        m_driver->set_gen_enabled((data.value & (USART_TXEN_bm | USART_RXEN_bm)) || m_ctrl->tx_in_progress());

        update_bitrate();
    }

    else if (reg_ofs== REG_OFS(CTRLC)) {
        //Extract the communication mode and deduct the clock mode
        ClockMode clk_mode;
        if (EXTRACT_GC(data.value, USART_CMODE) == USART_CMODE_ASYNCHRONOUS_gc)
            clk_mode = Clock_Async;
        else if (m_driver->gpio_controls(Line_XCK).dir)
            clk_mode = Clock_Emitter;
        else
            clk_mode = Clock_Receiver;

        m_ctrl->set_clock_mode(clk_mode);
        m_driver->set_line_enabled(Line_XCK, clk_mode != Clock_Async);

        //Frame format : extract the parity
        uint8_t pmode_reg = EXTRACT_GC(data.value, USART_PMODE);
        Parity pmode;
        if (pmode_reg == USART_PMODE_ODD_gc)
            pmode = Parity_Odd;
        else if (pmode_reg == USART_PMODE_EVEN_gc)
            pmode = Parity_Even;
        else
            pmode = Parity_No;

        m_ctrl->set_parity(pmode);

        //Frame format : extract the stop bits
        bool sbmode = EXTRACT_B(data.value, USART_SBMODE);
        m_ctrl->set_stopbits(sbmode ? 2 : 1);

        //Frame format : extract data bits
        uint8_t chsize = EXTRACT_F(data.value, USART_CHSIZE);
        if (chsize <= 3)
            m_ctrl->set_databits(chsize + 5);
        else if (chsize == (USART_CHSIZE_9BITL_gc >> USART_CHSIZE_gp) ||
                 chsize == (USART_CHSIZE_9BITH_gc >> USART_CHSIZE_gp))
            m_ctrl->set_databits(9);
        else
            m_ctrl->set_databits(8);

        update_bitrate();
    }

    else if (reg_ofs == REG_OFS(BAUDL) || reg_ofs == (REG_OFS(BAUDH))) {
        update_bitrate();
    }
}


void ArchXT_USART::ctrl_signal_raised(const signal_data_t& sigdata, int)
{
    if (sigdata.sigid == Signal_TX_Start) {
        //Notification that the pending frame has been pushed to the shift register
        //to be emitted. The TX buffer is now empty so raise the DRE interrupt.
        m_txe_intflag.set_flag();
        logger().dbg("TX started, raising DRE");
    }

    else if (sigdata.sigid == Signal_TX_Complete && sigdata.data.as_int()) {
        //Notification that the frame in the shift register has been emitted
        //Raise the TXC interrupt.
        m_txc_intflag.set_flag();
        logger().dbg("TX complete, raising TXC");
        //Case of a last transmission before disabling
        if (!TEST_IOREG(CTRLB, USART_TXEN) && !m_ctrl->tx_in_progress()) {
            m_driver->set_line_enabled(Line_TXD, false);
            m_driver->set_gen_enabled(TEST_IOREG(CTRLB, USART_RXEN));
        }
    }

    else if (sigdata.sigid == Signal_RX_Start) {
        logger().dbg("RX start");
        //If the Start-of-Frame detection is enabled, raise the RXS flag
        if (TEST_IOREG(CTRLB, USART_SFDEN) && device()->sleep_mode() == SleepMode::Standby) {
            m_rxc_intflag.set_flag(USART_RXSIF_bm);
            logger().dbg("Raising RXS");
        }
    }

    else if (sigdata.sigid == Signal_RX_Complete && sigdata.data.as_int()) {
        extract_rx_data();
        logger().dbg("RX complete");
    }

    else if (sigdata.sigid == Signal_RX_Overflow) {
        SET_IOREG(RXDATAH, USART_BUFOVF);
        logger().dbg("RX overflow detected");
    }
}


void ArchXT_USART::update_bitrate()
{
    cycle_count_t brr = (READ_IOREG(BAUDH) << 8) | READ_IOREG(BAUDL);
    if (brr < 64) brr = 64;

    uint16_t s;
    if (READ_IOREG_GC(CTRLC, USART_CMODE) == USART_CMODE_SYNCHRONOUS_gc)
        s = 2;
    else if (READ_IOREG_GC(CTRLB, USART_RXMODE) == USART_RXMODE_CLK2X_gc)
        s = 8;
    else
        s = 16;

    m_ctrl->set_bit_delay(s * brr / 64);
}


void ArchXT_USART::extract_rx_data()
{
    uint16_t data = m_ctrl->read_rx();
    WRITE_IOREG(RXDATAL, data & 0xFF);
    WRITE_IOREG_B(RXDATAH, USART_DATA8, (data >> 8) & 1);
    WRITE_IOREG_B(RXDATAH, USART_PERR, m_ctrl->has_parity_error());
    WRITE_IOREG_B(RXDATAH, USART_FERR, m_ctrl->has_frame_error());
    WRITE_IOREG_B(RXDATAH, USART_RXCIF, m_ctrl->rx_available());
    WRITE_IOREG_B(STATUS, USART_RXCIF, m_ctrl->rx_available());
    m_rxc_intflag.update_from_ioreg();
}

/*
* The USART is paused for modes above Standby and in Standby if the Start-Frame Detection feature is not enabled
*/
void ArchXT_USART::sleep(bool on, SleepMode mode)
{
    if (mode > SleepMode::Standby || (mode == SleepMode::Standby && !TEST_IOREG(CTRLB, USART_SFDEN))) {
        logger().dbg(on ? "Pausing" : "Resuming");
        m_ctrl->set_paused(on);
    }
}
