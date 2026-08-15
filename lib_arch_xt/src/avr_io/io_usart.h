/*
 * io_usart.h
 *
 *  Copyright 2026 Clement Savergne <csavergne@yahoo.com>

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

#ifndef _XT_IO_USART_H_
#define _XT_IO_USART_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the USART peripheral for the following device models:
 *  - atmega 0-series
 *  - attiny 0-series
 *  - attiny 1-series
 *  - attiny 2-series
 *
 *
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


/* USART - Universal Synchronous and Asynchronous Receiver and Transmitter */
/* USART.RXDATAL  bit masks and bit positions */
#define USART_DATA_gm  0xFF  /* RX Data group mask. */
#define USART_DATA_gp  0  /* RX Data group position. */
#define USART_DATA_0_bm  (1<<0)  /* RX Data bit 0 mask. */
#define USART_DATA_0_bp  0  /* RX Data bit 0 position. */
#define USART_DATA_1_bm  (1<<1)  /* RX Data bit 1 mask. */
#define USART_DATA_1_bp  1  /* RX Data bit 1 position. */
#define USART_DATA_2_bm  (1<<2)  /* RX Data bit 2 mask. */
#define USART_DATA_2_bp  2  /* RX Data bit 2 position. */
#define USART_DATA_3_bm  (1<<3)  /* RX Data bit 3 mask. */
#define USART_DATA_3_bp  3  /* RX Data bit 3 position. */
#define USART_DATA_4_bm  (1<<4)  /* RX Data bit 4 mask. */
#define USART_DATA_4_bp  4  /* RX Data bit 4 position. */
#define USART_DATA_5_bm  (1<<5)  /* RX Data bit 5 mask. */
#define USART_DATA_5_bp  5  /* RX Data bit 5 position. */
#define USART_DATA_6_bm  (1<<6)  /* RX Data bit 6 mask. */
#define USART_DATA_6_bp  6  /* RX Data bit 6 position. */
#define USART_DATA_7_bm  (1<<7)  /* RX Data bit 7 mask. */
#define USART_DATA_7_bp  7  /* RX Data bit 7 position. */

/* USART.RXDATAH  bit masks and bit positions */
#define USART_DATA8_bm  0x01  /* Receiver Data Register bit mask. */
#define USART_DATA8_bp  0  /* Receiver Data Register bit position. */
#define USART_PERR_bm  0x02  /* Parity Error bit mask. */
#define USART_PERR_bp  1  /* Parity Error bit position. */
#define USART_FERR_bm  0x04  /* Frame Error bit mask. */
#define USART_FERR_bp  2  /* Frame Error bit position. */
#define USART_BUFOVF_bm  0x40  /* Buffer Overflow bit mask. */
#define USART_BUFOVF_bp  6  /* Buffer Overflow bit position. */
#define USART_RXCIF_bm  0x80  /* Receive Complete Interrupt Flag bit mask. */
#define USART_RXCIF_bp  7  /* Receive Complete Interrupt Flag bit position. */

/* USART.TXDATAL  bit masks and bit positions */
/* USART_DATA  is already defined. */

/* USART.TXDATAH  bit masks and bit positions */
/* USART_DATA8  is already defined. */

/* USART.STATUS  bit masks and bit positions */
#define USART_WFB_bm  0x01  /* Wait For Break bit mask. */
#define USART_WFB_bp  0  /* Wait For Break bit position. */
#define USART_BDF_bm  0x02  /* Break Detected Flag bit mask. */
#define USART_BDF_bp  1  /* Break Detected Flag bit position. */
#define USART_ISFIF_bm  0x08  /* Inconsistent Sync Field Interrupt Flag bit mask. */
#define USART_ISFIF_bp  3  /* Inconsistent Sync Field Interrupt Flag bit position. */
#define USART_RXSIF_bm  0x10  /* Receive Start Interrupt bit mask. */
#define USART_RXSIF_bp  4  /* Receive Start Interrupt bit position. */
#define USART_DREIF_bm  0x20  /* Data Register Empty Flag bit mask. */
#define USART_DREIF_bp  5  /* Data Register Empty Flag bit position. */
#define USART_TXCIF_bm  0x40  /* Transmit Interrupt Flag bit mask. */
#define USART_TXCIF_bp  6  /* Transmit Interrupt Flag bit position. */
/* USART_RXCIF  is already defined. */

/* USART.CTRLA  bit masks and bit positions */
#define USART_RS485_gm  0x03  /* RS485 Mode internal transmitter group mask. */
#define USART_RS485_gp  0  /* RS485 Mode internal transmitter group position. */
#define USART_RS485_0_bm  (1<<0)  /* RS485 Mode internal transmitter bit 0 mask. */
#define USART_RS485_0_bp  0  /* RS485 Mode internal transmitter bit 0 position. */
#define USART_RS485_1_bm  (1<<1)  /* RS485 Mode internal transmitter bit 1 mask. */
#define USART_RS485_1_bp  1  /* RS485 Mode internal transmitter bit 1 position. */
#define USART_ABEIE_bm  0x04  /* Auto-baud Error Interrupt Enable bit mask. */
#define USART_ABEIE_bp  2  /* Auto-baud Error Interrupt Enable bit position. */
#define USART_LBME_bm  0x08  /* Loop-back Mode Enable bit mask. */
#define USART_LBME_bp  3  /* Loop-back Mode Enable bit position. */
#define USART_RXSIE_bm  0x10  /* Receiver Start Frame Interrupt Enable bit mask. */
#define USART_RXSIE_bp  4  /* Receiver Start Frame Interrupt Enable bit position. */
#define USART_DREIE_bm  0x20  /* Data Register Empty Interrupt Enable bit mask. */
#define USART_DREIE_bp  5  /* Data Register Empty Interrupt Enable bit position. */
#define USART_TXCIE_bm  0x40  /* Transmit Complete Interrupt Enable bit mask. */
#define USART_TXCIE_bp  6  /* Transmit Complete Interrupt Enable bit position. */
#define USART_RXCIE_bm  0x80  /* Receive Complete Interrupt Enable bit mask. */
#define USART_RXCIE_bp  7  /* Receive Complete Interrupt Enable bit position. */

/* USART.CTRLB  bit masks and bit positions */
#define USART_MPCM_bm  0x01  /* Multi-processor Communication Mode bit mask. */
#define USART_MPCM_bp  0  /* Multi-processor Communication Mode bit position. */
#define USART_RXMODE_gm  0x06  /* Receiver Mode group mask. */
#define USART_RXMODE_gp  1  /* Receiver Mode group position. */
#define USART_RXMODE_0_bm  (1<<1)  /* Receiver Mode bit 0 mask. */
#define USART_RXMODE_0_bp  1  /* Receiver Mode bit 0 position. */
#define USART_RXMODE_1_bm  (1<<2)  /* Receiver Mode bit 1 mask. */
#define USART_RXMODE_1_bp  2  /* Receiver Mode bit 1 position. */
#define USART_ODME_bm  0x08  /* Open Drain Mode Enable bit mask. */
#define USART_ODME_bp  3  /* Open Drain Mode Enable bit position. */
#define USART_SFDEN_bm  0x10  /* Start Frame Detection Enable bit mask. */
#define USART_SFDEN_bp  4  /* Start Frame Detection Enable bit position. */
#define USART_TXEN_bm  0x40  /* Transmitter Enable bit mask. */
#define USART_TXEN_bp  6  /* Transmitter Enable bit position. */
#define USART_RXEN_bm  0x80  /* Reciever enable bit mask. */
#define USART_RXEN_bp  7  /* Reciever enable bit position. */

/* USART.CTRLC  bit masks and bit positions */
#define USART_UCPHA_bm  0x02  /* SPI Host Mode, Clock Phase bit mask. */
#define USART_UCPHA_bp  1  /* SPI Host Mode, Clock Phase bit position. */
#define USART_UDORD_bm  0x04  /* SPI Host Mode, Data Order bit mask. */
#define USART_UDORD_bp  2  /* SPI Host Mode, Data Order bit position. */
#define USART_CHSIZE_gm  0x07  /* Character Size group mask. */
#define USART_CHSIZE_gp  0  /* Character Size group position. */
#define USART_CHSIZE_0_bm  (1<<0)  /* Character Size bit 0 mask. */
#define USART_CHSIZE_0_bp  0  /* Character Size bit 0 position. */
#define USART_CHSIZE_1_bm  (1<<1)  /* Character Size bit 1 mask. */
#define USART_CHSIZE_1_bp  1  /* Character Size bit 1 position. */
#define USART_CHSIZE_2_bm  (1<<2)  /* Character Size bit 2 mask. */
#define USART_CHSIZE_2_bp  2  /* Character Size bit 2 position. */
#define USART_SBMODE_bm  0x08  /* Stop Bit Mode bit mask. */
#define USART_SBMODE_bp  3  /* Stop Bit Mode bit position. */
#define USART_PMODE_gm  0x30  /* Parity Mode group mask. */
#define USART_PMODE_gp  4  /* Parity Mode group position. */
#define USART_PMODE_0_bm  (1<<4)  /* Parity Mode bit 0 mask. */
#define USART_PMODE_0_bp  4  /* Parity Mode bit 0 position. */
#define USART_PMODE_1_bm  (1<<5)  /* Parity Mode bit 1 mask. */
#define USART_PMODE_1_bp  5  /* Parity Mode bit 1 position. */
#define USART_CMODE_gm  0xC0  /* Communication Mode group mask. */
#define USART_CMODE_gp  6  /* Communication Mode group position. */
#define USART_CMODE_0_bm  (1<<6)  /* Communication Mode bit 0 mask. */
#define USART_CMODE_0_bp  6  /* Communication Mode bit 0 position. */
#define USART_CMODE_1_bm  (1<<7)  /* Communication Mode bit 1 mask. */
#define USART_CMODE_1_bp  7  /* Communication Mode bit 1 position. */

/* USART.CTRLD  bit masks and bit positions */
#define USART_ABW_gm  0xC0  /* Auto Baud Window group mask. */
#define USART_ABW_gp  6  /* Auto Baud Window group position. */
#define USART_ABW_0_bm  (1<<6)  /* Auto Baud Window bit 0 mask. */
#define USART_ABW_0_bp  6  /* Auto Baud Window bit 0 position. */
#define USART_ABW_1_bm  (1<<7)  /* Auto Baud Window bit 1 mask. */
#define USART_ABW_1_bp  7  /* Auto Baud Window bit 1 position. */

/* USART.DBGCTRL  bit masks and bit positions */
#define USART_DBGRUN_bm  0x01  /* Debug Run bit mask. */
#define USART_DBGRUN_bp  0  /* Debug Run bit position. */

/* USART.EVCTRL  bit masks and bit positions */
#define USART_IREI_bm  0x01  /* IrDA Event Input Enable bit mask. */
#define USART_IREI_bp  0  /* IrDA Event Input Enable bit position. */

/* USART.TXPLCTRL  bit masks and bit positions */
#define USART_TXPL_gm  0xFF  /* Transmit pulse length group mask. */
#define USART_TXPL_gp  0  /* Transmit pulse length group position. */
#define USART_TXPL_0_bm  (1<<0)  /* Transmit pulse length bit 0 mask. */
#define USART_TXPL_0_bp  0  /* Transmit pulse length bit 0 position. */
#define USART_TXPL_1_bm  (1<<1)  /* Transmit pulse length bit 1 mask. */
#define USART_TXPL_1_bp  1  /* Transmit pulse length bit 1 position. */
#define USART_TXPL_2_bm  (1<<2)  /* Transmit pulse length bit 2 mask. */
#define USART_TXPL_2_bp  2  /* Transmit pulse length bit 2 position. */
#define USART_TXPL_3_bm  (1<<3)  /* Transmit pulse length bit 3 mask. */
#define USART_TXPL_3_bp  3  /* Transmit pulse length bit 3 position. */
#define USART_TXPL_4_bm  (1<<4)  /* Transmit pulse length bit 4 mask. */
#define USART_TXPL_4_bp  4  /* Transmit pulse length bit 4 position. */
#define USART_TXPL_5_bm  (1<<5)  /* Transmit pulse length bit 5 mask. */
#define USART_TXPL_5_bp  5  /* Transmit pulse length bit 5 position. */
#define USART_TXPL_6_bm  (1<<6)  /* Transmit pulse length bit 6 mask. */
#define USART_TXPL_6_bp  6  /* Transmit pulse length bit 6 position. */
#define USART_TXPL_7_bm  (1<<7)  /* Transmit pulse length bit 7 mask. */
#define USART_TXPL_7_bp  7  /* Transmit pulse length bit 7 position. */

/* USART.RXPLCTRL  bit masks and bit positions */
#define USART_RXPL_gm  0x7F  /* Receiver Pulse Lenght group mask. */
#define USART_RXPL_gp  0  /* Receiver Pulse Lenght group position. */
#define USART_RXPL_0_bm  (1<<0)  /* Receiver Pulse Lenght bit 0 mask. */
#define USART_RXPL_0_bp  0  /* Receiver Pulse Lenght bit 0 position. */
#define USART_RXPL_1_bm  (1<<1)  /* Receiver Pulse Lenght bit 1 mask. */
#define USART_RXPL_1_bp  1  /* Receiver Pulse Lenght bit 1 position. */
#define USART_RXPL_2_bm  (1<<2)  /* Receiver Pulse Lenght bit 2 mask. */
#define USART_RXPL_2_bp  2  /* Receiver Pulse Lenght bit 2 position. */
#define USART_RXPL_3_bm  (1<<3)  /* Receiver Pulse Lenght bit 3 mask. */
#define USART_RXPL_3_bp  3  /* Receiver Pulse Lenght bit 3 position. */
#define USART_RXPL_4_bm  (1<<4)  /* Receiver Pulse Lenght bit 4 mask. */
#define USART_RXPL_4_bp  4  /* Receiver Pulse Lenght bit 4 position. */
#define USART_RXPL_5_bm  (1<<5)  /* Receiver Pulse Lenght bit 5 mask. */
#define USART_RXPL_5_bp  5  /* Receiver Pulse Lenght bit 5 position. */
#define USART_RXPL_6_bm  (1<<6)  /* Receiver Pulse Lenght bit 6 mask. */
#define USART_RXPL_6_bp  6  /* Receiver Pulse Lenght bit 6 position. */


/* Universal Synchronous and Asynchronous Receiver and Transmitter */
typedef struct USART_struct
{
    register8_t RXDATAL;  /* Receive Data Low Byte */
    register8_t RXDATAH;  /* Receive Data High Byte */
    register8_t TXDATAL;  /* Transmit Data Low Byte */
    register8_t TXDATAH;  /* Transmit Data High Byte */
    register8_t STATUS;  /* Status */
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t CTRLC;  /* Control C */
    _WORDREGISTER(BAUD);  /* Baud Rate */
    register8_t CTRLD;  /* Control D */
    register8_t DBGCTRL;  /* Debug Control */
    register8_t EVCTRL;  /* Event Control */
    register8_t TXPLCTRL;  /* IRCOM Transmitter Pulse Length Control */
    register8_t RXPLCTRL;  /* IRCOM Receiver Pulse Length Control */
    register8_t reserved_1[1];
} USART_t;

/* Auto Baud Window select bit group values */
typedef enum USART_ABW_VALUES_enum
{
    USART_ABW_WDW0_gv              = 0x00,  /* 18% tolerance */
    USART_ABW_WDW1_gv              = 0x01,  /* 15% tolerance */
    USART_ABW_WDW2_gv              = 0x02,  /* 21% tolerance */
    USART_ABW_WDW3_gv              = 0x03   /* 25% tolerance */
} USART_ABW_values_t;

/* Auto Baud Window select bit group configurations*/
typedef enum USART_ABW_enum
{
    USART_ABW_WDW0_gc              = (USART_ABW_WDW0_gv << USART_ABW_gp),  /* 18% tolerance */
    USART_ABW_WDW1_gc              = (USART_ABW_WDW1_gv << USART_ABW_gp),  /* 15% tolerance */
    USART_ABW_WDW2_gc              = (USART_ABW_WDW2_gv << USART_ABW_gp),  /* 21% tolerance */
    USART_ABW_WDW3_gc              = (USART_ABW_WDW3_gv << USART_ABW_gp)   /* 25% tolerance */
} USART_ABW_t;

/* Character Size select bit group values */
typedef enum USART_CHSIZE_VALUES_enum
{
    USART_CHSIZE_5BIT_gv           = 0x00,  /* Character size: 5 bit */
    USART_CHSIZE_6BIT_gv           = 0x01,  /* Character size: 6 bit */
    USART_CHSIZE_7BIT_gv           = 0x02,  /* Character size: 7 bit */
    USART_CHSIZE_8BIT_gv           = 0x03,  /* Character size: 8 bit */
    USART_CHSIZE_9BITL_gv          = 0x06,  /* Character size: 9 bit read low byte first */
    USART_CHSIZE_9BITH_gv          = 0x07   /* Character size: 9 bit read high byte first */
} USART_CHSIZE_values_t;

/* Character Size select bit group configurations*/
typedef enum USART_CHSIZE_enum
{
    USART_CHSIZE_5BIT_gc           = (USART_CHSIZE_5BIT_gv << USART_CHSIZE_gp),  /* Character size: 5 bit */
    USART_CHSIZE_6BIT_gc           = (USART_CHSIZE_6BIT_gv << USART_CHSIZE_gp),  /* Character size: 6 bit */
    USART_CHSIZE_7BIT_gc           = (USART_CHSIZE_7BIT_gv << USART_CHSIZE_gp),  /* Character size: 7 bit */
    USART_CHSIZE_8BIT_gc           = (USART_CHSIZE_8BIT_gv << USART_CHSIZE_gp),  /* Character size: 8 bit */
    USART_CHSIZE_9BITL_gc          = (USART_CHSIZE_9BITL_gv << USART_CHSIZE_gp),  /* Character size: 9 bit read low byte first */
    USART_CHSIZE_9BITH_gc          = (USART_CHSIZE_9BITH_gv << USART_CHSIZE_gp)   /* Character size: 9 bit read high byte first */
} USART_CHSIZE_t;

/* Communication Mode select bit group values */
typedef enum USART_CMODE_VALUES_enum
{
    USART_CMODE_ASYNCHRONOUS_gv    = 0x00,  /* Asynchronous Mode */
    USART_CMODE_SYNCHRONOUS_gv     = 0x01,  /* Synchronous Mode */
    USART_CMODE_IRCOM_gv           = 0x02,  /* Infrared Communication */
    USART_CMODE_MSPI_gv            = 0x03   /* SPI Host Mode */
} USART_CMODE_values_t;

/* Communication Mode select bit group configurations*/
typedef enum USART_CMODE_enum
{
    USART_CMODE_ASYNCHRONOUS_gc    = (USART_CMODE_ASYNCHRONOUS_gv << USART_CMODE_gp),  /* Asynchronous Mode */
    USART_CMODE_SYNCHRONOUS_gc     = (USART_CMODE_SYNCHRONOUS_gv << USART_CMODE_gp),  /* Synchronous Mode */
    USART_CMODE_IRCOM_gc           = (USART_CMODE_IRCOM_gv << USART_CMODE_gp),  /* Infrared Communication */
    USART_CMODE_MSPI_gc            = (USART_CMODE_MSPI_gv << USART_CMODE_gp)   /* SPI Host Mode */
} USART_CMODE_t;

/* Parity Mode select bit group values */
typedef enum USART_PMODE_VALUES_enum
{
    USART_PMODE_DISABLED_gv        = 0x00,  /* No Parity */
    USART_PMODE_EVEN_gv            = 0x02,  /* Even Parity */
    USART_PMODE_ODD_gv             = 0x03   /* Odd Parity */
} USART_PMODE_values_t;

/* Parity Mode select bit group configurations*/
typedef enum USART_PMODE_enum
{
    USART_PMODE_DISABLED_gc        = (USART_PMODE_DISABLED_gv << USART_PMODE_gp),  /* No Parity */
    USART_PMODE_EVEN_gc            = (USART_PMODE_EVEN_gv << USART_PMODE_gp),  /* Even Parity */
    USART_PMODE_ODD_gc             = (USART_PMODE_ODD_gv << USART_PMODE_gp)   /* Odd Parity */
} USART_PMODE_t;

/* RS485 Mode internal transmitter select bit group values */
typedef enum USART_RS485_VALUES_enum
{
    USART_RS485_OFF_gv             = 0x00,  /* RS485 Mode disabled */
    USART_RS485_EXT_gv             = 0x01,  /* RS485 Mode External drive */
    USART_RS485_INT_gv             = 0x02   /* RS485 Mode Internal drive */
} USART_RS485_values_t;

/* RS485 Mode internal transmitter select bit group configurations*/
typedef enum USART_RS485_enum
{
    USART_RS485_OFF_gc             = (USART_RS485_OFF_gv << USART_RS485_gp),  /* RS485 Mode disabled */
    USART_RS485_EXT_gc             = (USART_RS485_EXT_gv << USART_RS485_gp),  /* RS485 Mode External drive */
    USART_RS485_INT_gc             = (USART_RS485_INT_gv << USART_RS485_gp)   /* RS485 Mode Internal drive */
} USART_RS485_t;

/* Receiver Mode select bit group values */
typedef enum USART_RXMODE_VALUES_enum
{
    USART_RXMODE_NORMAL_gv         = 0x00,  /* Normal mode */
    USART_RXMODE_CLK2X_gv          = 0x01,  /* CLK2x mode */
    USART_RXMODE_GENAUTO_gv        = 0x02,  /* Generic autobaud mode */
    USART_RXMODE_LINAUTO_gv        = 0x03   /* LIN constrained autobaud mode */
} USART_RXMODE_values_t;

/* Receiver Mode select bit group configurations*/
typedef enum USART_RXMODE_enum
{
    USART_RXMODE_NORMAL_gc         = (USART_RXMODE_NORMAL_gv << USART_RXMODE_gp),  /* Normal mode */
    USART_RXMODE_CLK2X_gc          = (USART_RXMODE_CLK2X_gv << USART_RXMODE_gp),  /* CLK2x mode */
    USART_RXMODE_GENAUTO_gc        = (USART_RXMODE_GENAUTO_gv << USART_RXMODE_gp),  /* Generic autobaud mode */
    USART_RXMODE_LINAUTO_gc        = (USART_RXMODE_LINAUTO_gv << USART_RXMODE_gp)   /* LIN constrained autobaud mode */
} USART_RXMODE_t;

/* Stop Bit Mode select bit group values */
typedef enum USART_SBMODE_VALUES_enum
{
    USART_SBMODE_1BIT_gv           = 0x00,  /* 1 stop bit */
    USART_SBMODE_2BIT_gv           = 0x01   /* 2 stop bits */
} USART_SBMODE_values_t;

/* Stop Bit Mode select bit group configurations*/
typedef enum USART_SBMODE_enum
{
    USART_SBMODE_1BIT_gc           = (USART_SBMODE_1BIT_gv << USART_SBMODE_bp),  /* 1 stop bit */
    USART_SBMODE_2BIT_gc           = (USART_SBMODE_2BIT_gv << USART_SBMODE_bp)   /* 2 stop bits */
} USART_SBMODE_t;

/* End of extract */

#endif //_XT_IO_USART_H_
