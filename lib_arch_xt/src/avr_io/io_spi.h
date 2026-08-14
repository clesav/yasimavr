/*
 * io_spi.h
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

#ifndef _XT_IO_SPI_H_
#define _XT_IO_SPI_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the SPI peripheral for the following device models:
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


/* SPI - Serial Peripheral Interface */
/* SPI.CTRLA  bit masks and bit positions */
#define SPI_ENABLE_bm  0x01  /* Enable Module bit mask. */
#define SPI_ENABLE_bp  0  /* Enable Module bit position. */
#define SPI_PRESC_gm  0x06  /* Prescaler group mask. */
#define SPI_PRESC_gp  1  /* Prescaler group position. */
#define SPI_PRESC_0_bm  (1<<1)  /* Prescaler bit 0 mask. */
#define SPI_PRESC_0_bp  1  /* Prescaler bit 0 position. */
#define SPI_PRESC_1_bm  (1<<2)  /* Prescaler bit 1 mask. */
#define SPI_PRESC_1_bp  2  /* Prescaler bit 1 position. */
#define SPI_CLK2X_bm  0x10  /* Enable Double Speed bit mask. */
#define SPI_CLK2X_bp  4  /* Enable Double Speed bit position. */
#define SPI_MASTER_bm  0x20  /* Host Operation Enable bit mask. */
#define SPI_MASTER_bp  5  /* Host Operation Enable bit position. */
#define SPI_DORD_bm  0x40  /* Data Order Setting bit mask. */
#define SPI_DORD_bp  6  /* Data Order Setting bit position. */

/* SPI.CTRLB  bit masks and bit positions */
#define SPI_MODE_gm  0x03  /* SPI Mode group mask. */
#define SPI_MODE_gp  0  /* SPI Mode group position. */
#define SPI_MODE_0_bm  (1<<0)  /* SPI Mode bit 0 mask. */
#define SPI_MODE_0_bp  0  /* SPI Mode bit 0 position. */
#define SPI_MODE_1_bm  (1<<1)  /* SPI Mode bit 1 mask. */
#define SPI_MODE_1_bp  1  /* SPI Mode bit 1 position. */
#define SPI_SSD_bm  0x04  /* SPI Select Disable bit mask. */
#define SPI_SSD_bp  2  /* SPI Select Disable bit position. */
#define SPI_BUFWR_bm  0x40  /* Buffer Mode Wait for Receive bit mask. */
#define SPI_BUFWR_bp  6  /* Buffer Mode Wait for Receive bit position. */
#define SPI_BUFEN_bm  0x80  /* Buffer Mode Enable bit mask. */
#define SPI_BUFEN_bp  7  /* Buffer Mode Enable bit position. */

/* SPI.INTCTRL  bit masks and bit positions */
#define SPI_IE_bm  0x01  /* Interrupt Enable bit mask. */
#define SPI_IE_bp  0  /* Interrupt Enable bit position. */
#define SPI_SSIE_bm  0x10  /* SPI Select Trigger Interrupt Enable bit mask. */
#define SPI_SSIE_bp  4  /* SPI Select Trigger Interrupt Enable bit position. */
#define SPI_DREIE_bm  0x20  /* Data Register Empty Interrupt Enable bit mask. */
#define SPI_DREIE_bp  5  /* Data Register Empty Interrupt Enable bit position. */
#define SPI_TXCIE_bm  0x40  /* Transfer Complete Interrupt Enable bit mask. */
#define SPI_TXCIE_bp  6  /* Transfer Complete Interrupt Enable bit position. */
#define SPI_RXCIE_bm  0x80  /* Receive Complete Interrupt Enable bit mask. */
#define SPI_RXCIE_bp  7  /* Receive Complete Interrupt Enable bit position. */

/* SPI.INTFLAGS  bit masks and bit positions */
#define SPI_BUFOVF_bm  0x01  /* Buffer Overflow bit mask. */
#define SPI_BUFOVF_bp  0  /* Buffer Overflow bit position. */
#define SPI_SSIF_bm  0x10  /* SPI Select Trigger Interrupt Flag bit mask. */
#define SPI_SSIF_bp  4  /* SPI Select Trigger Interrupt Flag bit position. */
#define SPI_DREIF_bm  0x20  /* Data Register Empty Interrupt Flag bit mask. */
#define SPI_DREIF_bp  5  /* Data Register Empty Interrupt Flag bit position. */
#define SPI_TXCIF_bm  0x40  /* Transfer Complete Interrupt Flag bit mask. */
#define SPI_TXCIF_bp  6  /* Transfer Complete Interrupt Flag bit position. */
#define SPI_WRCOL_bm  0x40  /* Write Collision bit mask. */
#define SPI_WRCOL_bp  6  /* Write Collision bit position. */
#define SPI_RXCIF_bm  0x80  /* Receive Complete Interrupt Flag bit mask. */
#define SPI_RXCIF_bp  7  /* Receive Complete Interrupt Flag bit position. */
#define SPI_IF_bm  0x80  /* Interrupt Flag bit mask. */
#define SPI_IF_bp  7  /* Interrupt Flag bit position. */


/* Serial Peripheral Interface */
typedef struct SPI_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t DATA;  /* Data */
    register8_t reserved_1[3];
} SPI_t;

/* SPI Mode select bit group values */
typedef enum SPI_MODE_VALUES_enum
{
    SPI_MODE_0_gv                  = 0x00,  /* SPI Mode 0 */
    SPI_MODE_1_gv                  = 0x01,  /* SPI Mode 1 */
    SPI_MODE_2_gv                  = 0x02,  /* SPI Mode 2 */
    SPI_MODE_3_gv                  = 0x03   /* SPI Mode 3 */
} SPI_MODE_values_t;

/* SPI Mode select bit group configurations*/
typedef enum SPI_MODE_enum
{
    SPI_MODE_0_gc                  = (SPI_MODE_0_gv << SPI_MODE_gp),  /* SPI Mode 0 */
    SPI_MODE_1_gc                  = (SPI_MODE_1_gv << SPI_MODE_gp),  /* SPI Mode 1 */
    SPI_MODE_2_gc                  = (SPI_MODE_2_gv << SPI_MODE_gp),  /* SPI Mode 2 */
    SPI_MODE_3_gc                  = (SPI_MODE_3_gv << SPI_MODE_gp)   /* SPI Mode 3 */
} SPI_MODE_t;

/* Prescaler select bit group values */
typedef enum SPI_PRESC_VALUES_enum
{
    SPI_PRESC_DIV4_gv              = 0x00,  /* CLK_PER / 4 */
    SPI_PRESC_DIV16_gv             = 0x01,  /* CLK_PER / 16 */
    SPI_PRESC_DIV64_gv             = 0x02,  /* CLK_PER / 64 */
    SPI_PRESC_DIV128_gv            = 0x03   /* CLK_PER / 128 */
} SPI_PRESC_values_t;

/* Prescaler select bit group configurations*/
typedef enum SPI_PRESC_enum
{
    SPI_PRESC_DIV4_gc              = (SPI_PRESC_DIV4_gv << SPI_PRESC_gp),  /* CLK_PER / 4 */
    SPI_PRESC_DIV16_gc             = (SPI_PRESC_DIV16_gv << SPI_PRESC_gp),  /* CLK_PER / 16 */
    SPI_PRESC_DIV64_gc             = (SPI_PRESC_DIV64_gv << SPI_PRESC_gp),  /* CLK_PER / 64 */
    SPI_PRESC_DIV128_gc            = (SPI_PRESC_DIV128_gv << SPI_PRESC_gp)   /* CLK_PER / 128 */
} SPI_PRESC_t;

/* End of extract */

#endif //_XT_IO_SPI_H_
