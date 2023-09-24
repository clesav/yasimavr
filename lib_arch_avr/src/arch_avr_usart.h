/*
 * arch_avr_usart.h
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

#ifndef __YASIMAVR_AVR_USART_H__
#define __YASIMAVR_AVR_USART_H__

#include "arch_avr_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_uart.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_uart
   \brief Configuration structure for ArchAVR_USART
 */
struct ArchAVR_USARTConfig {

    reg_addr_t reg_data;            ///< Data register address

    regbit_t rb_rx_enable;          ///< RX enable bit
    regbit_t rb_tx_enable;          ///< TX enable bit
    regbit_t rb_rxc_inten;          ///< RXC interrupt enable bit
    regbit_t rb_rxc_flag;           ///< RXC flag bit
    regbit_t rb_txc_inten;          ///< TXC interrupt enable bit
    regbit_t rb_txc_flag;           ///< TXC flag bit
    regbit_t rb_txe_inten;          ///< TXE (DRE) interrupt enable bit
    regbit_t rb_txe_flag;           ///< TXE flag bit

    regbit_t rb_baud_2x;            ///< double bitrate bit
    reg_addr_t reg_baud;            ///< bitrate register (1 or 2 bytes)
    uint8_t baud_bitsize;           ///< size of the bitrate field (in bits)

    int_vect_t rxc_vector;          ///< RXC interrupt vector
    int_vect_t txc_vector;          ///< TXC interrupt vector
    int_vect_t txe_vector;          ///< TXE (DRE) interrupt vector

};


/**
   \ingroup api_uart
   \brief Implementation of a USART interface for AVR series

   Supported features:
    - 8-bits frames only, regardless of the frame size setting
    - stop bits and parity settings have no effect
    - synchronous, SPI and MPCM modes are not supported
    - RXC, TXC, UDRE interrupts are supported
    - Error flags are not supported

    CTLREQs supported:
     - AVR_CTLREQ_GET_SIGNAL : returns in data.p the signal of the underlying
        UART (see sim_uart.h)
     - AVR_CTLREQ_UART_ENDPOINT : returns in data.p the endpoint to use in order to transmit
        data in and out (see sim_uart.h)
 */
class AVR_ARCHAVR_PUBLIC_API ArchAVR_USART : public Peripheral, public SignalHook {

public:

    ArchAVR_USART(uint8_t num, const ArchAVR_USARTConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchAVR_USARTConfig& m_config;

    UART m_uart;
    UARTEndPoint m_endpoint;

    InterruptFlag m_rxc_intflag;
    InterruptFlag m_txc_intflag;
    InterruptFlag m_txe_intflag;

    void update_framerate();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_AVR_USART_H__
