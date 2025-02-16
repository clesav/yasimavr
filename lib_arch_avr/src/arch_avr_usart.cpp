/*
 * arch_avr_usart.cpp
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

#include "arch_avr_usart.h"
#include "ioctrl_common/sim_uart.h"
#include "core/sim_pin.h"
#include "core/sim_device.h"
#include <algorithm>

YASIMAVR_USING_NAMESPACE

using namespace UART;


//=======================================================================================

class ArchAVR_USART::_PinDriver : public PinDriver {

public:

    _PinDriver(ArchAVR_USART& per);
    void reset();
    void set_line_enabled(Line line, bool enabled);
    void set_line_state(Line line, bool state);
    bool get_line_state(Line line) const;

    virtual Pin::controls_t override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls) override;
    virtual void digital_state_changed(pin_index_t pin_index, bool dig_state) override;

private:

    ArchAVR_USART& m_peripheral;
    bool m_line_states[3];
    bool m_line_enabled[3];

};


class ArchAVR_USART::_Controller : public USART {

public:

    _Controller(ArchAVR_USART& per) : m_peripheral(per) {}

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

    ArchAVR_USART& m_peripheral;

};


//=======================================================================================

static const bool initial_line_states[] = { true, true, false };


ArchAVR_USART::_PinDriver::_PinDriver(ArchAVR_USART& per)
:PinDriver(per.id(), 3)
,m_peripheral(per)
,m_line_enabled{ false, false, false }
{
    std::copy(std::begin(initial_line_states), std::end(initial_line_states), m_line_states);
    std::fill(std::begin(m_line_enabled), std::end(m_line_enabled), false);
}


void ArchAVR_USART::_PinDriver::reset()
{
    std::copy(std::begin(initial_line_states), std::end(initial_line_states), m_line_states);
    std::fill(std::begin(m_line_enabled), std::end(m_line_enabled), false);
    set_enabled(false);
}


void ArchAVR_USART::_PinDriver::set_line_enabled(Line line, bool enabled)
{
    m_line_enabled[line] = enabled;
    set_enabled(m_line_enabled[Line_TXD] || m_line_enabled[Line_RXD]);
    update_pin_state(line);
}


void ArchAVR_USART::_PinDriver::set_line_state(Line line, bool state)
{
    if (line < Line_DIR) {
        m_line_states[line] = state;
        update_pin_state(line);
    }
}


bool ArchAVR_USART::_PinDriver::get_line_state(Line line) const
{
    if (line < Line_DIR)
        return m_line_states[line];
    else
        return false;
}


Pin::controls_t ArchAVR_USART::_PinDriver::override_gpio(pin_index_t pin_index, const Pin::controls_t& gpio_controls)
{
    Pin::controls_t c = gpio_controls;
    if (m_line_enabled[pin_index]) {
        switch ((Line) pin_index) {
            case Line_TXD: {
                c.dir = 1;
                c.drive = m_line_states[Line_TXD];
            } break;

            case Line_RXD: {
                c.dir = 0;
            } break;

            case Line_XCK: {
                c.dir = 1;
                c.drive = m_line_states[Line_XCK];
            } break;

            default: break;
        }
    }

    return c;
}


void ArchAVR_USART::_PinDriver::digital_state_changed(pin_index_t pin_index, bool dig_state)
{
    m_line_states[pin_index] = dig_state;
    m_peripheral.m_ctrl->line_state_changed((Line) pin_index, dig_state);
}


//=======================================================================================

ArchAVR_USART::ArchAVR_USART(uint8_t num, const ArchAVR_USARTConfig& config)
:Peripheral(AVR_IOCTL_UART(0x30 + num))
,m_config(config)
,m_ctrl_hook(*this, &ArchAVR_USART::ctrl_signal_raised)
,m_rxc_intflag(false)
,m_txc_intflag(true)
,m_txe_intflag(false)
{
    m_driver = new _PinDriver(*this);
    m_ctrl = new _Controller(*this);
}


ArchAVR_USART::~ArchAVR_USART()
{
    delete m_driver;
    delete m_ctrl;
}


bool ArchAVR_USART::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.rbc_rx_data, true);
    add_ioreg(m_config.rbc_tx_data);
    add_ioreg(m_config.rb_rx_enable);
    add_ioreg(m_config.rb_tx_enable);
    add_ioreg(m_config.rb_rxc_inten);
    add_ioreg(m_config.rb_rxc_flag, true);
    add_ioreg(m_config.rb_txc_inten);
    add_ioreg(m_config.rb_txc_flag);
    add_ioreg(m_config.rb_txe_inten);
    add_ioreg(m_config.rb_txe_flag, true);
    add_ioreg(m_config.rb_baud_2x);
    add_ioreg(m_config.rbc_baud);
    add_ioreg(m_config.rb_ferr, true);
    add_ioreg(m_config.rb_overrun, true);
    add_ioreg(m_config.rb_perr, true);
    add_ioreg(m_config.rbc_chsize);
    add_ioreg(m_config.rb_clock_mode);
    add_ioreg(m_config.rb_parity);
    add_ioreg(m_config.rb_stopbits);

    status &= m_rxc_intflag.init(device,
                                 m_config.rb_rxc_inten,
                                 m_config.rb_rxc_flag,
                                 m_config.iv_rxc);
    status &= m_txc_intflag.init(device,
                                 m_config.rb_txc_inten,
                                 m_config.rb_txc_flag,
                                 m_config.iv_txc);
    status &= m_txe_intflag.init(device,
                                 m_config.rb_txe_inten,
                                 m_config.rb_txe_flag,
                                 m_config.iv_txe);

    m_ctrl->init(*device.cycle_manager(), logger());
    m_ctrl->set_tx_buffer_limit(2);
    m_ctrl->set_rx_buffer_limit(3);
    m_ctrl->signal().connect(m_ctrl_hook);

    device.pin_manager().register_driver(*m_driver);

    return status;
}


void ArchAVR_USART::reset()
{
    m_driver->reset();
    m_ctrl->reset();
    set_ioreg(m_config.rb_txe_flag);
    write_ioreg(m_config.rbc_chsize, 3); //8-bits by default
    update_bitrate();
}


bool ArchAVR_USART::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
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


uint8_t ArchAVR_USART::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == m_config.rbc_rx_data[0].addr) {
        m_ctrl->pop_rx();
        extract_rx_data();
        if (!m_ctrl->rx_available())
            m_rxc_intflag.clear_flag();
    }

    return value;
}


void ArchAVR_USART::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //Writing data to the DATA register trigger a emit.
    if (addr == m_config.rbc_rx_data[0].addr && test_ioreg(m_config.rb_tx_enable)) {
        m_txe_intflag.clear_flag();
        m_ctrl->push_tx(read_ioreg(m_config.rbc_tx_data));
    }

    if (addr == m_config.rb_tx_enable.addr) {
        if (m_config.rb_tx_enable.extract(data.posedge())) {
            m_driver->set_line_enabled(Line_TXD, true);
        }
        //Writing 0 to TXE cancels any pending TX but wait until the current TX is complete
        //before disabling the interface
        else if (m_config.rb_tx_enable.extract(data.negedge())) {
            m_ctrl->cancel_tx_pending();
            m_txe_intflag.set_flag();
            if (!m_ctrl->tx_in_progress())
                m_driver->set_line_enabled(Line_TXD, false);
        }
    }

    //Writing to RXE
    if (addr == m_config.rb_rx_enable.addr) {
        bool enabled = m_config.rb_rx_enable.extract(data.value);
        m_driver->set_line_enabled(Line_RXD, enabled);
        m_ctrl->set_rx_enabled(enabled);
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

    if (m_config.rbc_chsize.addr_match(addr)) {
        uint8_t reg_chsize = read_ioreg(m_config.rbc_chsize);
        unsigned short chsize = (reg_chsize < 4) ? (reg_chsize + 5) : 9;
        m_ctrl->set_databits(chsize);
    }

    if (addr == m_config.rb_stopbits.addr) {
        uint8_t sb = m_config.rb_stopbits.extract(data.value);
        m_ctrl->set_stopbits(sb ? 2 : 1);
    }

    if (addr == m_config.rb_parity.addr) {
        uint8_t p = m_config.rb_parity.extract(data.value);
        if (p == 2)
            m_ctrl->set_parity(Parity_Even);
        else if (p == 3)
            m_ctrl->set_parity(Parity_Odd);
        else
            m_ctrl->set_parity(Parity_No);
    }

    //Modification of the frame rate
    if (m_config.rbc_baud.addr_match(addr) || addr == m_config.rb_baud_2x.addr)
        update_bitrate();
}


void ArchAVR_USART::ctrl_signal_raised(const signal_data_t& sigdata, int)
{
    //If a frame emission is started, it means the TX buffer is empty
    //so raise the TXE (DRE) flag
    if (sigdata.sigid == Signal_TX_Start) {
        m_txe_intflag.set_flag();
    }

    //If a frame is successfully emitted, raise the TXC flag
    else if (sigdata.sigid == Signal_TX_Complete && sigdata.data.as_int()) {
        m_txc_intflag.set_flag();
        if (!test_ioreg(m_config.rb_tx_enable))
            m_driver->set_line_enabled(Line_TXD, false);
    }

    //If a frame is successfully received, raise the RXC flag
    else if (sigdata.sigid == Signal_RX_Complete && sigdata.data.as_int()) {
        m_rxc_intflag.set_flag();
        extract_rx_data();
    }
}


void ArchAVR_USART::update_bitrate()
{
    //Prescaler counter value
    uint16_t brr = read_ioreg(m_config.rbc_baud);
    //baudrate calculation, as per the datasheet
    cycle_count_t bit_delay;
    if (test_ioreg(m_config.rb_baud_2x))
        bit_delay = (brr + 1) << 3;
    else
        bit_delay = (brr + 1) << 4;

    logger().dbg("Baud rate set to %d bps", (device()->frequency() / bit_delay));

    m_ctrl->set_bit_delay(bit_delay);
}


void ArchAVR_USART::extract_rx_data()
{
    uint16_t data = m_ctrl->read_rx();
    write_ioreg(m_config.rbc_rx_data, data);
    write_ioreg(m_config.rb_ferr, m_ctrl->has_frame_error());
    write_ioreg(m_config.rb_perr, m_ctrl->has_parity_error());
    write_ioreg(m_config.rb_overrun, m_ctrl->has_rx_overrun());
}
