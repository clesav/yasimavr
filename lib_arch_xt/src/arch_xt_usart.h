/*
 * arch_xt_usart.h
 *
 *  Copyright 2022 Clement Savergne <csavergne@yahoo.com>

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

#ifndef __YASIMAVR_XT_USART_H__
#define __YASIMAVR_XT_USART_H__

#include "arch_xt_globals.h"
#include "core/sim_interrupt.h"
#include "ioctrl_common/sim_uart.h"

YASIMAVR_BEGIN_NAMESPACE


//=======================================================================================

/**
   \ingroup api_uart
   \brief Configuration structure for ArchXT_USART.
 */
struct ArchXT_USARTConfig {

    /// Base address for the peripheral I/O registers
    reg_addr_t reg_base;
    /// Interrupt vector index for RXC
    int_vect_t iv_rxc;
    /// Interrupt vector index for TXC
    int_vect_t iv_txc;
    /// Interrupt vector index for TXE
    int_vect_t iv_txe;

};

/**
   \ingroup api_uart
   \brief Implementation of a USART interface for XT core series

   Features:
    - 8-bits frames only, regardless of the frame size setting
    - stop bits and parity settings have no effect
    - synchronous, SPI MPCM modes are not supported
    - one-wire / RS485 and IRCOM modes not supported
    - RXC, RXS, TXC, UDRE interrupts are supported
    - Start-of-Frame detection is supported
    - Error flags are not supported
    - Auto-baud not supported

   CTLREQs supported:
    - AVR_CTLREQ_GET_SIGNAL : returns in data.p the signal of the underlying
      UART (see sim_uart.h)
    - AVR_CTLREQ_UART_ENDPOINT : returns in data.p the endpoint to use in order to transmit
      data in and out (see sim_uart.h)
 */
class AVR_ARCHXT_PUBLIC_API ArchXT_USART : public Peripheral, public SignalHook {

public:

    ArchXT_USART(uint8_t num, const ArchXT_USARTConfig& config);

    virtual bool init(Device& device) override;
    virtual void reset() override;
    virtual bool ctlreq(ctlreq_id_t req, ctlreq_data_t* data) override;
    virtual uint8_t ioreg_read_handler(reg_addr_t addr, uint8_t value) override;
    virtual void ioreg_write_handler(reg_addr_t addr, const ioreg_write_t& data) override;
    virtual void sleep(bool on, SleepMode mode) override;
    virtual void raised(const signal_data_t& sigdata, int hooktag) override;

private:

    const ArchXT_USARTConfig& m_config;

    UART m_uart;
    UARTEndPoint m_endpoint;

    InterruptFlag m_rxc_intflag;
    InterruptFlag m_txc_intflag;
    InterruptFlag m_txe_intflag;

    void update_framerate();

};


YASIMAVR_END_NAMESPACE

#endif //__YASIMAVR_XT_USART_H__
