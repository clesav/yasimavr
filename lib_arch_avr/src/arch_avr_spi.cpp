/*
 * arch_avr_spi.h
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

#include "arch_avr_spi.h"
#include "core/sim_pin.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define HOOKTAG_SPI         0
#define HOOKTAG_PIN         1

const uint32_t ClockFactors[] = {4, 16, 64, 128};


ArchAVR_SPI::ArchAVR_SPI(uint8_t num, const ArchAVR_SPIConfig& config)
:Peripheral(AVR_IOCTL_SPI(0x30 + num))
,m_config(config)
,m_pin_select(nullptr)
,m_pin_selected(false)
,m_intflag(true)
{}

bool ArchAVR_SPI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(m_config.reg_data);
    add_ioreg(m_config.rb_enable);
    add_ioreg(m_config.rb_int_enable);
    add_ioreg(m_config.rb_int_flag, true);
    add_ioreg(m_config.rb_mode);
    add_ioreg(m_config.rb_clock);
    add_ioreg(m_config.rb_clock2x);

    status &= m_intflag.init(device,
                             m_config.rb_int_enable,
                             m_config.rb_int_flag,
                             m_config.iv_spi);

    m_spi.init(*device.cycle_manager(), logger());
    m_spi.set_tx_buffer_limit(1);
    m_spi.set_rx_buffer_limit(2);
    m_spi.signal().connect(*this, HOOKTAG_SPI);

    m_pin_select = device.find_pin(m_config.pin_select);
    m_pin_select->signal().connect(*this, HOOKTAG_PIN);

    return status;
}

void ArchAVR_SPI::reset()
{
    m_spi.reset();
    m_pin_selected = false;
    update_framerate();
}

bool ArchAVR_SPI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
{
    if (req == AVR_CTLREQ_GET_SIGNAL) {
        data->data = &m_spi.signal();
        return true;
    }
    else if (req == AVR_CTLREQ_SPI_ADD_CLIENT) {
        SPIClient* client = reinterpret_cast<SPIClient*>(data->data.as_ptr());
        if (client)
            m_spi.add_client(*client);
        return true;
    }
    else if (req == AVR_CTLREQ_SPI_CLIENT) {
        data->data = &m_spi;
        return true;
    }
    else if (req == AVR_CTLREQ_SPI_SELECT) {
        m_pin_selected = (data->data.as_uint() > 0);
        m_spi.set_selected(m_pin_selected && test_ioreg(m_config.rb_enable));
        return true;
    }

    return false;
}

uint8_t ArchAVR_SPI::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    if (addr == m_config.reg_data) {
        value = m_spi.pop_rx();
        if (!m_spi.rx_available())
            m_intflag.clear_flag();
    }

    return value;
}

void ArchAVR_SPI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    //Writing data to the DATA register with SPE set triggers a transfer.
    if (addr == m_config.reg_data && test_ioreg(m_config.rb_enable)) {
        m_spi.push_tx(data.value);
        m_intflag.clear_flag();
    }

    //Writing 0 to SPE cancels any pending TX
    if (addr == m_config.rb_enable.addr) {
        m_spi.set_selected(m_pin_selected && m_config.rb_enable.extract(data.value));
        if (m_config.rb_enable.extract(data.negedge()))
            m_spi.cancel_tx();
    }

    //Writing to SPIE
    if (addr == m_config.rb_int_enable.addr)
        m_intflag.update_from_ioreg();

    //Modification of the frame rate
    if (addr == m_config.rb_clock.addr || addr == m_config.rb_clock2x.addr)
        update_framerate();

    //Writing to the mode selection bit
    if (addr == m_config.rb_mode.addr)
        m_spi.set_host_mode(m_config.rb_mode.extract(data.value));
}

void ArchAVR_SPI::raised(const signal_data_t& sigdata, int hooktag)
{
    if (sigdata.sigid != Pin::Signal_DigitalChange) return;

    //On completion of a transfer, raise the interrupt flag
    if (hooktag == HOOKTAG_SPI) {
        if (sigdata.sigid == SPI::Signal_HostTfrComplete ||
            sigdata.sigid == SPI::Signal_ClientTfrComplete)
            m_intflag.set_flag();
    }
    //Signal of pin state change, check if we're selected
    else if (hooktag == HOOKTAG_PIN) {
        m_pin_selected = !sigdata.data.as_uint();
        m_spi.set_selected(m_pin_selected && test_ioreg(m_config.rb_enable));
    }
}

void ArchAVR_SPI::update_framerate()
{
    uint8_t clk_setting = read_ioreg(m_config.rb_clock);
    uint32_t clk_factor = ClockFactors[clk_setting];

    if (test_ioreg(m_config.rb_clock2x))
        clk_factor >>= 1;

    m_spi.set_frame_delay(clk_factor * 8);
}
