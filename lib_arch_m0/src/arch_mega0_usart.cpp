/*
 * arch_mega0_usart.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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

#include "arch_mega0_usart.h"
#include "arch_mega0_io.h"
#include "arch_mega0_io_utils.h"


//=======================================================================================

#define REG_ADDR(reg) \
	reg_addr_t(m_config.reg_base + offsetof(USART_t, reg))

#define REG_OFS(reg) \
	offsetof(USART_t, reg)


AVR_ArchMega0_USART::AVR_ArchMega0_USART(uint8_t num, const AVR_ArchMega0_USART_Config& config)
:AVR_Peripheral(AVR_IOCTL_UART(0x30 + num))
,m_config(config)
,m_rxc_intflag(false)
,m_txc_intflag(false)
,m_txe_intflag(false)
{
	m_endpoint = { &m_uart.signal(), &m_uart };
}

bool AVR_ArchMega0_USART::init(AVR_Device& device)
{
	bool status = AVR_Peripheral::init(device);

	add_ioreg(REG_ADDR(RXDATAL), USART_DATA_gm, true);
	add_ioreg(REG_ADDR(RXDATAH), USART_RXCIF_bm, true);
	add_ioreg(REG_ADDR(TXDATAL), USART_DATA_gm);
	//TXDATAH not implemented
	add_ioreg(REG_ADDR(STATUS), USART_RXCIF_bm | USART_DREIF_bm, true); // R/O part
	add_ioreg(REG_ADDR(STATUS), USART_TXCIF_bm); // R/W part
	add_ioreg(REG_ADDR(CTRLA), USART_RXCIE_bm | USART_TXCIE_bm | USART_DREIE_bm);
	add_ioreg(REG_ADDR(CTRLB), USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_gm);
	add_ioreg(REG_ADDR(CTRLC), 0);
	add_ioreg(REG_ADDR(BAUDL));
	add_ioreg(REG_ADDR(BAUDH));
	//CTRLD not implemented
	//DBGCTRL not supported
	//EVCTRL not implemented
	//TXPLCTRL not implemented
	//RXPLCTRL not implemented

	status &= m_rxc_intflag.init(device,
								 DEF_REGBIT_B(CTRLA, USART_RXCIE),
								 DEF_REGBIT_B(STATUS, USART_RXCIF),
								 m_config.iv_rxc);
	status &= m_txc_intflag.init(device,
							     DEF_REGBIT_B(CTRLA, USART_TXCIE),
							     DEF_REGBIT_B(STATUS, USART_TXCIF),
								 m_config.iv_txc);
	status &= m_txe_intflag.init(device,
		     	 	 	 	 	 DEF_REGBIT_B(CTRLA, USART_DREIE),
								 DEF_REGBIT_B(STATUS, USART_DREIF),
								 m_config.iv_txe);

	m_uart.init(device.cycle_manager(), device.logger());
	m_uart.set_tx_buffer_limit(2);
	m_uart.set_rx_buffer_limit(3);
	m_uart.signal().connect_hook(this);

	return status;
}

void AVR_ArchMega0_USART::reset()
{
	m_uart.reset();
	SET_IOREG(STATUS, USART_DREIF);
	update_framerate();
}

bool AVR_ArchMega0_USART::ctlreq(uint16_t req, ctlreq_data_t *data)
{
	if (req == AVR_CTLREQ_GET_SIGNAL) {
		data->p = &m_uart.signal();
		return true;
	}
	else if (req == AVR_CTLREQ_UART_ENDPOINT) {
		data->p = &m_endpoint;
		return true;
	}

	return false;
}

void AVR_ArchMega0_USART::ioreg_read_handler(reg_addr_t addr)
{
	reg_addr_t reg_ofs = addr - m_config.reg_base;

	if (reg_ofs == REG_OFS(RXDATAL)) {
		uint8_t frame = m_uart.pop_rx();
		write_ioreg(REG_ADDR(RXDATAL), frame);
		if (!m_uart.rx_available())
			m_rxc_intflag.clear_flag();
	}
}

void AVR_ArchMega0_USART::ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data)
{
	reg_addr_t reg_ofs = addr - m_config.reg_base;

	//Writing to TXDATA emits the value, if TX is enabled
	if (reg_ofs == REG_OFS(TXDATAL)) {
		if (TEST_IOREG(CTRLB, USART_TXEN)) {
			m_uart.push_tx(data.value);
			m_txe_intflag.clear_flag();
		}
	}

	else if (reg_ofs == REG_OFS(STATUS)) {
		//The whole register is made of write-one-to-clear bits
		write_ioreg(REG_ADDR(STATUS), data.old);
		//Writing one to TXCIF clears the bit and cancels the interrupt
		if (data.value & USART_TXCIF_bm)
			m_txc_intflag.clear_flag();
	}

	else if (reg_ofs == REG_OFS(CTRLA)) {
		m_txc_intflag.update_from_ioreg();
		m_txe_intflag.update_from_ioreg();
		m_rxc_intflag.update_from_ioreg();
	}

	else if (reg_ofs == REG_OFS(CTRLB)) {
		//Processing of TXEN. If it is cleared, we flush the TX buffer
		if (data.negedge & USART_TXEN_bm) {
			m_uart.cancel_tx_pending();
			m_txe_intflag.set_flag();
		}

		//Processing of RXEN changes
		if (data.posedge & USART_RXEN_bm) {
			m_uart.set_rx_enabled(true);
		}
		else if (data.negedge & USART_RXEN_bm) {
			m_uart.set_rx_enabled(false);
			m_rxc_intflag.clear_flag();
		}

		update_framerate();
	}

	else if (reg_ofs == REG_OFS(BAUD) || reg_ofs == (REG_OFS(BAUD) + 1)) {
		update_framerate();
	}
}

void AVR_ArchMega0_USART::raised(const signal_data_t& data, uint16_t id)
{
	if (data.index == UART_TX_Start)
		//Notification that the pending frame has been pushed to the shift register
		//to be emitted. The TX buffer is now empty so raise the DRE interrupt.
		m_txe_intflag.set_flag();

	else if (data.index == UART_TX_Complete && data.u)
		//Notification that the frame in the shift register has been emitted
		//Raise the TXC interrupt.
		m_txc_intflag.set_flag();

	else if (data.index == UART_RX_Complete && data.u)
		//Raise the RX completion flag
		m_rxc_intflag.set_flag();
}

void AVR_ArchMega0_USART::update_framerate()
{
	//From datasheet (normal speed mode) : (Fbaud = Fclk * 64 / (16 * reg_baud))
	//Expressed in delay rather than frequency: Tbaud / Tclk = reg_baud / 4
	uint16_t brr = (read_ioreg(REG_ADDR(BAUD) + 1) << 8) | read_ioreg(REG_ADDR(BAUD));
	uint32_t delay = (brr >> 2);
	m_uart.set_frame_delay(delay);
}

