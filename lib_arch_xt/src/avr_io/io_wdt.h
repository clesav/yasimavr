/*
 * io_wdt.h
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

#ifndef _XT_IO_WDT_H_
#define _XT_IO_WDT_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the WDT peripheral for the following device models:
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


/* WDT - Watch-Dog Timer */
/* WDT.CTRLA  bit masks and bit positions */
#define WDT_PERIOD_gm  0x0F  /* Period group mask. */
#define WDT_PERIOD_gp  0  /* Period group position. */
#define WDT_PERIOD_0_bm  (1<<0)  /* Period bit 0 mask. */
#define WDT_PERIOD_0_bp  0  /* Period bit 0 position. */
#define WDT_PERIOD_1_bm  (1<<1)  /* Period bit 1 mask. */
#define WDT_PERIOD_1_bp  1  /* Period bit 1 position. */
#define WDT_PERIOD_2_bm  (1<<2)  /* Period bit 2 mask. */
#define WDT_PERIOD_2_bp  2  /* Period bit 2 position. */
#define WDT_PERIOD_3_bm  (1<<3)  /* Period bit 3 mask. */
#define WDT_PERIOD_3_bp  3  /* Period bit 3 position. */
#define WDT_WINDOW_gm  0xF0  /* Window group mask. */
#define WDT_WINDOW_gp  4  /* Window group position. */
#define WDT_WINDOW_0_bm  (1<<4)  /* Window bit 0 mask. */
#define WDT_WINDOW_0_bp  4  /* Window bit 0 position. */
#define WDT_WINDOW_1_bm  (1<<5)  /* Window bit 1 mask. */
#define WDT_WINDOW_1_bp  5  /* Window bit 1 position. */
#define WDT_WINDOW_2_bm  (1<<6)  /* Window bit 2 mask. */
#define WDT_WINDOW_2_bp  6  /* Window bit 2 position. */
#define WDT_WINDOW_3_bm  (1<<7)  /* Window bit 3 mask. */
#define WDT_WINDOW_3_bp  7  /* Window bit 3 position. */

/* WDT.STATUS  bit masks and bit positions */
#define WDT_SYNCBUSY_bm  0x01  /* Syncronization busy bit mask. */
#define WDT_SYNCBUSY_bp  0  /* Syncronization busy bit position. */
#define WDT_LOCK_bm  0x80  /* Lock enable bit mask. */
#define WDT_LOCK_bp  7  /* Lock enable bit position. */


/* Watch-Dog Timer */
typedef struct WDT_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t STATUS;  /* Status */
} WDT_t;

/* Period select bit group values */
typedef enum WDT_PERIOD_VALUES_enum
{
    WDT_PERIOD_OFF_gv              = 0x00,  /* Off */
    WDT_PERIOD_8CLK_gv             = 0x01,  /* 8 cycles (8ms) */
    WDT_PERIOD_16CLK_gv            = 0x02,  /* 16 cycles (16ms) */
    WDT_PERIOD_32CLK_gv            = 0x03,  /* 32 cycles (32ms) */
    WDT_PERIOD_64CLK_gv            = 0x04,  /* 64 cycles (64ms) */
    WDT_PERIOD_128CLK_gv           = 0x05,  /* 128 cycles (0.128s) */
    WDT_PERIOD_256CLK_gv           = 0x06,  /* 256 cycles (0.256s) */
    WDT_PERIOD_512CLK_gv           = 0x07,  /* 512 cycles (0.512s) */
    WDT_PERIOD_1KCLK_gv            = 0x08,  /* 1K cycles (1.0s) */
    WDT_PERIOD_2KCLK_gv            = 0x09,  /* 2K cycles (2.0s) */
    WDT_PERIOD_4KCLK_gv            = 0x0A,  /* 4K cycles (4.1s) */
    WDT_PERIOD_8KCLK_gv            = 0x0B   /* 8K cycles (8.2s) */
} WDT_PERIOD_values_t;

/* Period select bit group configurations*/
typedef enum WDT_PERIOD_enum
{
    WDT_PERIOD_OFF_gc              = (WDT_PERIOD_OFF_gv << WDT_PERIOD_gp),  /* Off */
    WDT_PERIOD_8CLK_gc             = (WDT_PERIOD_8CLK_gv << WDT_PERIOD_gp),  /* 8 cycles (8ms) */
    WDT_PERIOD_16CLK_gc            = (WDT_PERIOD_16CLK_gv << WDT_PERIOD_gp),  /* 16 cycles (16ms) */
    WDT_PERIOD_32CLK_gc            = (WDT_PERIOD_32CLK_gv << WDT_PERIOD_gp),  /* 32 cycles (32ms) */
    WDT_PERIOD_64CLK_gc            = (WDT_PERIOD_64CLK_gv << WDT_PERIOD_gp),  /* 64 cycles (64ms) */
    WDT_PERIOD_128CLK_gc           = (WDT_PERIOD_128CLK_gv << WDT_PERIOD_gp),  /* 128 cycles (0.128s) */
    WDT_PERIOD_256CLK_gc           = (WDT_PERIOD_256CLK_gv << WDT_PERIOD_gp),  /* 256 cycles (0.256s) */
    WDT_PERIOD_512CLK_gc           = (WDT_PERIOD_512CLK_gv << WDT_PERIOD_gp),  /* 512 cycles (0.512s) */
    WDT_PERIOD_1KCLK_gc            = (WDT_PERIOD_1KCLK_gv << WDT_PERIOD_gp),  /* 1K cycles (1.0s) */
    WDT_PERIOD_2KCLK_gc            = (WDT_PERIOD_2KCLK_gv << WDT_PERIOD_gp),  /* 2K cycles (2.0s) */
    WDT_PERIOD_4KCLK_gc            = (WDT_PERIOD_4KCLK_gv << WDT_PERIOD_gp),  /* 4K cycles (4.1s) */
    WDT_PERIOD_8KCLK_gc            = (WDT_PERIOD_8KCLK_gv << WDT_PERIOD_gp)   /* 8K cycles (8.2s) */
} WDT_PERIOD_t;

/* Window select bit group values */
typedef enum WDT_WINDOW_VALUES_enum
{
    WDT_WINDOW_OFF_gv              = 0x00,  /* Off */
    WDT_WINDOW_8CLK_gv             = 0x01,  /* 8 cycles (8ms) */
    WDT_WINDOW_16CLK_gv            = 0x02,  /* 16 cycles (16ms) */
    WDT_WINDOW_32CLK_gv            = 0x03,  /* 32 cycles (32ms) */
    WDT_WINDOW_64CLK_gv            = 0x04,  /* 64 cycles (64ms) */
    WDT_WINDOW_128CLK_gv           = 0x05,  /* 128 cycles (0.128s) */
    WDT_WINDOW_256CLK_gv           = 0x06,  /* 256 cycles (0.256s) */
    WDT_WINDOW_512CLK_gv           = 0x07,  /* 512 cycles (0.512s) */
    WDT_WINDOW_1KCLK_gv            = 0x08,  /* 1K cycles (1.0s) */
    WDT_WINDOW_2KCLK_gv            = 0x09,  /* 2K cycles (2.0s) */
    WDT_WINDOW_4KCLK_gv            = 0x0A,  /* 4K cycles (4.1s) */
    WDT_WINDOW_8KCLK_gv            = 0x0B   /* 8K cycles (8.2s) */
} WDT_WINDOW_values_t;

/* Window select bit group configurations*/
typedef enum WDT_WINDOW_enum
{
    WDT_WINDOW_OFF_gc              = (WDT_WINDOW_OFF_gv << WDT_WINDOW_gp),  /* Off */
    WDT_WINDOW_8CLK_gc             = (WDT_WINDOW_8CLK_gv << WDT_WINDOW_gp),  /* 8 cycles (8ms) */
    WDT_WINDOW_16CLK_gc            = (WDT_WINDOW_16CLK_gv << WDT_WINDOW_gp),  /* 16 cycles (16ms) */
    WDT_WINDOW_32CLK_gc            = (WDT_WINDOW_32CLK_gv << WDT_WINDOW_gp),  /* 32 cycles (32ms) */
    WDT_WINDOW_64CLK_gc            = (WDT_WINDOW_64CLK_gv << WDT_WINDOW_gp),  /* 64 cycles (64ms) */
    WDT_WINDOW_128CLK_gc           = (WDT_WINDOW_128CLK_gv << WDT_WINDOW_gp),  /* 128 cycles (0.128s) */
    WDT_WINDOW_256CLK_gc           = (WDT_WINDOW_256CLK_gv << WDT_WINDOW_gp),  /* 256 cycles (0.256s) */
    WDT_WINDOW_512CLK_gc           = (WDT_WINDOW_512CLK_gv << WDT_WINDOW_gp),  /* 512 cycles (0.512s) */
    WDT_WINDOW_1KCLK_gc            = (WDT_WINDOW_1KCLK_gv << WDT_WINDOW_gp),  /* 1K cycles (1.0s) */
    WDT_WINDOW_2KCLK_gc            = (WDT_WINDOW_2KCLK_gv << WDT_WINDOW_gp),  /* 2K cycles (2.0s) */
    WDT_WINDOW_4KCLK_gc            = (WDT_WINDOW_4KCLK_gv << WDT_WINDOW_gp),  /* 4K cycles (4.1s) */
    WDT_WINDOW_8KCLK_gc            = (WDT_WINDOW_8KCLK_gv << WDT_WINDOW_gp)   /* 8K cycles (8.2s) */
} WDT_WINDOW_t;

/* End of extract */

#endif //_XT_IO_WDT_H_
