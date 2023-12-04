/*
 * arch_xt_spi.cpp
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

#include "arch_xt_spi.h"
#include "arch_xt_io.h"
#include "arch_xt_io_utils.h"

YASIMAVR_USING_NAMESPACE


//=======================================================================================

#define REG_ADDR(reg) \
    reg_addr_t(m_config.reg_base + offsetof(SPI_t, reg))

#define REG_OFS(reg) \
    offsetof(SPI_t, reg)


#define HOOKTAG_SPI         0
#define HOOKTAG_PIN         1

const unsigned long ClockFactors[] = {4, 16, 64, 128};


//=======================================================================================

ArchXT_SPI::ArchXT_SPI(int num, const ArchXT_SPIConfig& config)
:Peripheral(AVR_IOCTL_SPI(0x30 + num))
,m_config(config)
,m_pin_select(nullptr)
,m_pin_selected(false)
,m_intflag(false)
{}

bool ArchXT_SPI::init(Device& device)
{
    bool status = Peripheral::init(device);

    add_ioreg(REG_ADDR(CTRLA), SPI_DORD_bm | SPI_MASTER_bm | SPI_CLK2X_bm |
                               SPI_PRESC_gm | SPI_ENABLE_bm);
    add_ioreg(REG_ADDR(CTRLB), SPI_SSD_bm | SPI_MODE_gm);
    add_ioreg(REG_ADDR(INTCTRL), SPI_IE_bm);
    add_ioreg(REG_ADDR(INTFLAGS), SPI_IF_bm);
    add_ioreg(REG_ADDR(DATA));

    status &= m_intflag.init(device,
                             DEF_REGBIT_B(INTCTRL, SPI_IE),
                             DEF_REGBIT_B(INTFLAGS, SPI_IF),
                             m_config.iv_spi);

    m_spi.init(*device.cycle_manager(), logger());
    m_spi.set_tx_buffer_limit(1);
    m_spi.set_rx_buffer_limit(2);
    m_spi.signal().connect(*this, HOOKTAG_SPI);

    if (m_config.pin_select) {
        m_pin_select = device.find_pin(m_config.pin_select);
        if (!m_pin_select)
            m_pin_select->signal().connect(*this, HOOKTAG_PIN);
        else
            status = false;
    }

    return status;
}

void ArchXT_SPI::reset()
{
    m_spi.reset();
    m_pin_selected = false;
    m_intflag.update_from_ioreg();
}

bool ArchXT_SPI::ctlreq(ctlreq_id_t req, ctlreq_data_t* data)
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
        m_spi.set_selected(m_pin_selected && TEST_IOREG(CTRLA, SPI_ENABLE));
        return true;
    }

    return false;
}

uint8_t ArchXT_SPI::ioreg_read_handler(reg_addr_t addr, uint8_t value)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(DATA)) {
        value = m_spi.pop_rx();
        if (!m_spi.rx_available())
            m_intflag.clear_flag();
    }

    return value;
}

void ArchXT_SPI::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
    reg_addr_t reg_ofs = addr - m_config.reg_base;

    if (reg_ofs == REG_OFS(CTRLA)) {
        m_spi.set_host_mode(data.value & SPI_MASTER_bm);
        m_spi.set_selected((data.value & SPI_ENABLE_bm) && m_pin_selected);

        uint8_t clk_setting = EXTRACT_F(data.value, SPI_PRESC);
        unsigned long clk_factor = ClockFactors[clk_setting];
        if (data.value & SPI_CLK2X_bm)
            clk_factor >>= 1;
        m_spi.set_frame_delay(clk_factor * 8);
    }

    //Writing to DATA emits the value, if TX is enabled
    else if (reg_ofs == REG_OFS(INTCTRL)) {
        m_intflag.update_from_ioreg();
    }

    else if (reg_ofs == REG_OFS(INTFLAGS)) {
        write_ioreg(REG_ADDR(INTFLAGS), data.old);
        if (data.value & SPI_IF_bm)
            m_intflag.clear_flag();
    }

    else if (reg_ofs == REG_OFS(DATA)) {
        m_spi.push_tx(data.value);
    }
}

void ArchXT_SPI::raised(const signal_data_t& sigdata, int hooktag)
{
    if (hooktag == HOOKTAG_SPI) {
        if (sigdata.sigid == SPI::Signal_HostTfrComplete ||
            sigdata.sigid == SPI::Signal_ClientTfrComplete)
            m_intflag.set_flag();
    }
    else if (hooktag == HOOKTAG_PIN) {
        if (sigdata.sigid != Pin::Signal_DigitalChange) {
            m_pin_selected = !sigdata.data.as_uint();
            m_spi.set_selected(m_pin_selected && TEST_IOREG(CTRLA, SPI_ENABLE));
        }
    }
}
