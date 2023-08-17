/*
 * arch_avr_usart.cpp
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

#include "arch_avr_usart.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

ArchAVR_USART::ArchAVR_USART(uint8_t num, const ArchAVR_USARTConfig& config)
:Peripheral(AVR_IOCTL_UART(0x30 + num))
,m_config(config)
,m_rxc_intflag(false)
,m_txc_intflag(true)
,m_txe_intflag(false)
{
    m_endpoint = { &m_uart.signal(), &m_uart };
}

bool ArchAVR_USART::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.reg_data);
    add_ioreg(m_config.rb_rx_enable);
    add_ioreg(m_config.rb_tx_enable);
    add_ioreg(m_config.rb_rxc_inten);
    add_ioreg(m_config.rb_rxc_flag, true);
    add_ioreg(m_config.rb_txc_inten);
    add_ioreg(m_config.rb_txc_flag);
    add_ioreg(m_config.rb_txe_inten);
    add_ioreg(m_config.rb_txe_flag, true);
    add_ioreg(m_config.rb_baud_2x);

    uint16_t baud_bitmask = (1 << m_config.baud_bitsize) - 1;
    add_ioreg(m_config.reg_baud, baud_bitmask & 0xFF);
    if (m_config.baud_bitsize > 8)
        add_ioreg(m_config.reg_baud + 1, baud_bitmask >> 8);

    status &= m_rxc_intflag.init(device,
                                 m_config.rb_rxc_inten,
                                 m_config.rb_rxc_flag,
                                 m_config.rxc_vector);
    status &= m_txc_intflag.init(device,
                                 m_config.rb_txc_inten,
                                 m_config.rb_txc_flag,
                                 m_config.txc_vector);
    status &= m_txe_intflag.init(device,
                                 m_config.rb_txe_inten,
                                 m_config.rb_txe_flag,
                                 m_config.txe_vector);

    m_uart.init(*device.cycle_manager(), logger());
    m_uart.set_tx_buffer_limit(2);
    m_uart.set_rx_buffer_limit(3);
    m_uart.signal().connect(*this);

    return status;
}

void ArchAVR_USART::reset()
{
    m_uart.reset();
    set_ioreg(m_config.rb_txe_flag);
    update_framerate();
}

bool ArchAVR_USART::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_uart.signal();
        return true;
    }
    else if (req == AVR_CTLREQ_UART_ENDPOINT) {
        data->data = &m_endpoint;
        return true;
    }

    return false;
}

uint8_t ArchAVR_USART::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == m_config.reg_data) {
        value = m_uart.pop_rx();
        if (!m_uart.rx_available())
            m_rxc_intflag.clear_flag();
    }

    return value;
}

void ArchAVR_USART::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //Writing data to the DATA register trigger a emit.
    if (addr == m_config.reg_data && test_ioreg(m_config.rb_tx_enable)) {
        m_txe_intflag.clear_flag();
        m_uart.push_tx(data.value);
    }

    //Writing 0 to TXE cancels any pending TX
    if (addr == m_config.rb_tx_enable.addr && m_config.rb_tx_enable.extract(data.negedge())) {
        m_uart.cancel_tx_pending();
        m_txe_intflag.set_flag();
    }

    //Writing to RXE
    if (addr == m_config.rb_rx_enable.addr) {
        bool enabled = m_config.rb_rx_enable.extract(data.value);
        m_uart.set_rx_enabled(enabled);
        if (!enabled)
            m_rxc_intflag.clear_flag();
    }

    //Writing to TXCIE
    if (addr == m_config.rb_txc_inten.addr)
        m_txc_intflag.update_from_ioreg();

    //Writing 1 to TXC clears the bit and cancels the interrupt
    if (addr == m_config.rb_txc_flag.addr && m_config.rb_txc_flag.extract(data.value))
        m_txc_intflag.clear_flag();

    //Writing to TXEIE (a.k.a. UDREIE)
    if (addr == m_config.rb_txe_inten.addr)
        m_txe_intflag.update_from_ioreg();

    //Writing to RXCIE
    if (addr == m_config.rb_rxc_inten.addr)
        m_rxc_intflag.update_from_ioreg();

    //Modification of the frame rate
    if (addr == m_config.reg_baud || addr == m_config.rb_baud_2x.addr)
        update_framerate();

}

void ArchAVR_USART::raised(const signal_data_t& sigdata, int)
{
    //If a frame emission is started, it means the TX buffer is empty
    //so raise the TXE (DRE) flag
    if (sigdata.sigid == UART::Signal_TX_Start)
        m_txe_intflag.set_flag();

    //If a frame is successfully emitted, raise the TXC flag
    else if (sigdata.sigid == UART::Signal_TX_Complete && sigdata.data.as_int())
        m_txc_intflag.set_flag();

    //If a frame is successfully received, raise the RXC flag
    else if (sigdata.sigid == UART::Signal_RX_Complete && sigdata.data.as_int())
        m_rxc_intflag.set_flag();
}

void ArchAVR_USART::update_framerate()
{
    //Prescaler counter value
    uint16_t brr = read_ioreg(m_config.reg_baud);
    if (m_config.baud_bitsize > 8)
        brr |= read_ioreg(m_config.reg_baud + 1) << 8;

    //baudrate calculation, as per the datasheet
    uint32_t bit_delay;
    if (test_ioreg(m_config.rb_baud_2x))
        bit_delay = (brr + 1) << 3;
    else
        bit_delay = (brr + 1) << 4;

    logger().dbg("Baud rate set to %d bps", (device()->frequency() / bit_delay));

    //The USART frame delay is for 10-bits frames (8-bits data, no parity, 1 stop bit)
    uint32_t frame_delay = bit_delay * 10;

    m_uart.set_frame_delay(frame_delay);
}
