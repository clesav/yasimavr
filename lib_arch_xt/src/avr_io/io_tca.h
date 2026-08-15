/*
 * io_tca.h
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

#ifndef _XT_IO_TCA_H_
#define _XT_IO_TCA_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the TCA peripheral for the following device models:
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


/* TCA - 16-bit Timer/Counter Type A */
/* TCA_SINGLE.CTRLA  bit masks and bit positions */
#define TCA_SINGLE_ENABLE_bm  0x01  /* Module Enable bit mask. */
#define TCA_SINGLE_ENABLE_bp  0  /* Module Enable bit position. */
#define TCA_SINGLE_CLKSEL_gm  0x0E  /* Clock Selection group mask. */
#define TCA_SINGLE_CLKSEL_gp  1  /* Clock Selection group position. */
#define TCA_SINGLE_CLKSEL_0_bm  (1<<1)  /* Clock Selection bit 0 mask. */
#define TCA_SINGLE_CLKSEL_0_bp  1  /* Clock Selection bit 0 position. */
#define TCA_SINGLE_CLKSEL_1_bm  (1<<2)  /* Clock Selection bit 1 mask. */
#define TCA_SINGLE_CLKSEL_1_bp  2  /* Clock Selection bit 1 position. */
#define TCA_SINGLE_CLKSEL_2_bm  (1<<3)  /* Clock Selection bit 2 mask. */
#define TCA_SINGLE_CLKSEL_2_bp  3  /* Clock Selection bit 2 position. */
#define TCA_SINGLE_RUNSTDBY_bm  0x80  /* Run in Standby bit mask. */
#define TCA_SINGLE_RUNSTDBY_bp  7  /* Run in Standby bit position. */

/* TCA_SINGLE.CTRLB  bit masks and bit positions */
#define TCA_SINGLE_WGMODE_gm  0x07  /* Waveform generation mode group mask. */
#define TCA_SINGLE_WGMODE_gp  0  /* Waveform generation mode group position. */
#define TCA_SINGLE_WGMODE_0_bm  (1<<0)  /* Waveform generation mode bit 0 mask. */
#define TCA_SINGLE_WGMODE_0_bp  0  /* Waveform generation mode bit 0 position. */
#define TCA_SINGLE_WGMODE_1_bm  (1<<1)  /* Waveform generation mode bit 1 mask. */
#define TCA_SINGLE_WGMODE_1_bp  1  /* Waveform generation mode bit 1 position. */
#define TCA_SINGLE_WGMODE_2_bm  (1<<2)  /* Waveform generation mode bit 2 mask. */
#define TCA_SINGLE_WGMODE_2_bp  2  /* Waveform generation mode bit 2 position. */
#define TCA_SINGLE_ALUPD_bm  0x08  /* Auto Lock Update bit mask. */
#define TCA_SINGLE_ALUPD_bp  3  /* Auto Lock Update bit position. */
#define TCA_SINGLE_CMP0EN_bm  0x10  /* Compare 0 Enable bit mask. */
#define TCA_SINGLE_CMP0EN_bp  4  /* Compare 0 Enable bit position. */
#define TCA_SINGLE_CMP1EN_bm  0x20  /* Compare 1 Enable bit mask. */
#define TCA_SINGLE_CMP1EN_bp  5  /* Compare 1 Enable bit position. */
#define TCA_SINGLE_CMP2EN_bm  0x40  /* Compare 2 Enable bit mask. */
#define TCA_SINGLE_CMP2EN_bp  6  /* Compare 2 Enable bit position. */

/* TCA_SINGLE.CTRLC  bit masks and bit positions */
#define TCA_SINGLE_CMP0OV_bm  0x01  /* Compare 0 Waveform Output Value bit mask. */
#define TCA_SINGLE_CMP0OV_bp  0  /* Compare 0 Waveform Output Value bit position. */
#define TCA_SINGLE_CMP1OV_bm  0x02  /* Compare 1 Waveform Output Value bit mask. */
#define TCA_SINGLE_CMP1OV_bp  1  /* Compare 1 Waveform Output Value bit position. */
#define TCA_SINGLE_CMP2OV_bm  0x04  /* Compare 2 Waveform Output Value bit mask. */
#define TCA_SINGLE_CMP2OV_bp  2  /* Compare 2 Waveform Output Value bit position. */

/* TCA_SINGLE.CTRLD  bit masks and bit positions */
#define TCA_SINGLE_SPLITM_bm  0x01  /* Split Mode Enable bit mask. */
#define TCA_SINGLE_SPLITM_bp  0  /* Split Mode Enable bit position. */

/* TCA_SINGLE.CTRLECLR  bit masks and bit positions */
#define TCA_SINGLE_DIR_bm  0x01  /* Direction bit mask. */
#define TCA_SINGLE_DIR_bp  0  /* Direction bit position. */
#define TCA_SINGLE_LUPD_bm  0x02  /* Lock Update bit mask. */
#define TCA_SINGLE_LUPD_bp  1  /* Lock Update bit position. */
#define TCA_SINGLE_CMD_gm  0x0C  /* Command group mask. */
#define TCA_SINGLE_CMD_gp  2  /* Command group position. */
#define TCA_SINGLE_CMD_0_bm  (1<<2)  /* Command bit 0 mask. */
#define TCA_SINGLE_CMD_0_bp  2  /* Command bit 0 position. */
#define TCA_SINGLE_CMD_1_bm  (1<<3)  /* Command bit 1 mask. */
#define TCA_SINGLE_CMD_1_bp  3  /* Command bit 1 position. */

/* TCA_SINGLE.CTRLESET  bit masks and bit positions */
/* TCA_SINGLE_DIR  is already defined. */
/* TCA_SINGLE_LUPD  is already defined. */
/* TCA_SINGLE_CMD  is already defined. */

/* TCA_SINGLE.CTRLFCLR  bit masks and bit positions */
#define TCA_SINGLE_PERBV_bm  0x01  /* Period Buffer Valid bit mask. */
#define TCA_SINGLE_PERBV_bp  0  /* Period Buffer Valid bit position. */
#define TCA_SINGLE_CMP0BV_bm  0x02  /* Compare 0 Buffer Valid bit mask. */
#define TCA_SINGLE_CMP0BV_bp  1  /* Compare 0 Buffer Valid bit position. */
#define TCA_SINGLE_CMP1BV_bm  0x04  /* Compare 1 Buffer Valid bit mask. */
#define TCA_SINGLE_CMP1BV_bp  2  /* Compare 1 Buffer Valid bit position. */
#define TCA_SINGLE_CMP2BV_bm  0x08  /* Compare 2 Buffer Valid bit mask. */
#define TCA_SINGLE_CMP2BV_bp  3  /* Compare 2 Buffer Valid bit position. */

/* TCA_SINGLE.CTRLFSET  bit masks and bit positions */
/* TCA_SINGLE_PERBV  is already defined. */
/* TCA_SINGLE_CMP0BV  is already defined. */
/* TCA_SINGLE_CMP1BV  is already defined. */
/* TCA_SINGLE_CMP2BV  is already defined. */

/* TCA_SINGLE.EVCTRL  bit masks and bit positions */
#define TCA_SINGLE_CNTAEI_bm  0x01  /* Count on Event Input A bit mask. */
#define TCA_SINGLE_CNTAEI_bp  0  /* Count on Event Input A bit position. */
#define TCA_SINGLE_EVACTA_gm  0x0E  /* Event Action A group mask. */
#define TCA_SINGLE_EVACTA_gp  1  /* Event Action A group position. */
#define TCA_SINGLE_EVACTA_0_bm  (1<<1)  /* Event Action A bit 0 mask. */
#define TCA_SINGLE_EVACTA_0_bp  1  /* Event Action A bit 0 position. */
#define TCA_SINGLE_EVACTA_1_bm  (1<<2)  /* Event Action A bit 1 mask. */
#define TCA_SINGLE_EVACTA_1_bp  2  /* Event Action A bit 1 position. */
#define TCA_SINGLE_EVACTA_2_bm  (1<<3)  /* Event Action A bit 2 mask. */
#define TCA_SINGLE_EVACTA_2_bp  3  /* Event Action A bit 2 position. */
#define TCA_SINGLE_CNTBEI_bm  0x10  /* Count on Event Input B bit mask. */
#define TCA_SINGLE_CNTBEI_bp  4  /* Count on Event Input B bit position. */
#define TCA_SINGLE_EVACTB_gm  0xE0  /* Event Action B group mask. */
#define TCA_SINGLE_EVACTB_gp  5  /* Event Action B group position. */
#define TCA_SINGLE_EVACTB_0_bm  (1<<5)  /* Event Action B bit 0 mask. */
#define TCA_SINGLE_EVACTB_0_bp  5  /* Event Action B bit 0 position. */
#define TCA_SINGLE_EVACTB_1_bm  (1<<6)  /* Event Action B bit 1 mask. */
#define TCA_SINGLE_EVACTB_1_bp  6  /* Event Action B bit 1 position. */
#define TCA_SINGLE_EVACTB_2_bm  (1<<7)  /* Event Action B bit 2 mask. */
#define TCA_SINGLE_EVACTB_2_bp  7  /* Event Action B bit 2 position. */

/* TCA_SINGLE.INTCTRL  bit masks and bit positions */
#define TCA_SINGLE_OVF_bm  0x01  /* Overflow Interrupt bit mask. */
#define TCA_SINGLE_OVF_bp  0  /* Overflow Interrupt bit position. */
#define TCA_SINGLE_CMP0_bm  0x10  /* Compare 0 Interrupt bit mask. */
#define TCA_SINGLE_CMP0_bp  4  /* Compare 0 Interrupt bit position. */
#define TCA_SINGLE_CMP1_bm  0x20  /* Compare 1 Interrupt bit mask. */
#define TCA_SINGLE_CMP1_bp  5  /* Compare 1 Interrupt bit position. */
#define TCA_SINGLE_CMP2_bm  0x40  /* Compare 2 Interrupt bit mask. */
#define TCA_SINGLE_CMP2_bp  6  /* Compare 2 Interrupt bit position. */

/* TCA_SINGLE.INTFLAGS  bit masks and bit positions */
/* TCA_SINGLE_OVF  is already defined. */
/* TCA_SINGLE_CMP0  is already defined. */
/* TCA_SINGLE_CMP1  is already defined. */
/* TCA_SINGLE_CMP2  is already defined. */

/* TCA_SINGLE.DBGCTRL  bit masks and bit positions */
#define TCA_SINGLE_DBGRUN_bm  0x01  /* Debug Run bit mask. */
#define TCA_SINGLE_DBGRUN_bp  0  /* Debug Run bit position. */

/* TCA_SPLIT.CTRLA  bit masks and bit positions */
#define TCA_SPLIT_ENABLE_bm  0x01  /* Module Enable bit mask. */
#define TCA_SPLIT_ENABLE_bp  0  /* Module Enable bit position. */
#define TCA_SPLIT_CLKSEL_gm  0x0E  /* Clock Selection group mask. */
#define TCA_SPLIT_CLKSEL_gp  1  /* Clock Selection group position. */
#define TCA_SPLIT_CLKSEL_0_bm  (1<<1)  /* Clock Selection bit 0 mask. */
#define TCA_SPLIT_CLKSEL_0_bp  1  /* Clock Selection bit 0 position. */
#define TCA_SPLIT_CLKSEL_1_bm  (1<<2)  /* Clock Selection bit 1 mask. */
#define TCA_SPLIT_CLKSEL_1_bp  2  /* Clock Selection bit 1 position. */
#define TCA_SPLIT_CLKSEL_2_bm  (1<<3)  /* Clock Selection bit 2 mask. */
#define TCA_SPLIT_CLKSEL_2_bp  3  /* Clock Selection bit 2 position. */
#define TCA_SPLIT_RUNSTDBY_bm  0x80  /* Run in Standby bit mask. */
#define TCA_SPLIT_RUNSTDBY_bp  7  /* Run in Standby bit position. */

/* TCA_SPLIT.CTRLB  bit masks and bit positions */
#define TCA_SPLIT_LCMP0EN_bm  0x01  /* Low Compare 0 Enable bit mask. */
#define TCA_SPLIT_LCMP0EN_bp  0  /* Low Compare 0 Enable bit position. */
#define TCA_SPLIT_LCMP1EN_bm  0x02  /* Low Compare 1 Enable bit mask. */
#define TCA_SPLIT_LCMP1EN_bp  1  /* Low Compare 1 Enable bit position. */
#define TCA_SPLIT_LCMP2EN_bm  0x04  /* Low Compare 2 Enable bit mask. */
#define TCA_SPLIT_LCMP2EN_bp  2  /* Low Compare 2 Enable bit position. */
#define TCA_SPLIT_HCMP0EN_bm  0x10  /* High Compare 0 Enable bit mask. */
#define TCA_SPLIT_HCMP0EN_bp  4  /* High Compare 0 Enable bit position. */
#define TCA_SPLIT_HCMP1EN_bm  0x20  /* High Compare 1 Enable bit mask. */
#define TCA_SPLIT_HCMP1EN_bp  5  /* High Compare 1 Enable bit position. */
#define TCA_SPLIT_HCMP2EN_bm  0x40  /* High Compare 2 Enable bit mask. */
#define TCA_SPLIT_HCMP2EN_bp  6  /* High Compare 2 Enable bit position. */

/* TCA_SPLIT.CTRLC  bit masks and bit positions */
#define TCA_SPLIT_LCMP0OV_bm  0x01  /* Low Compare 0 Output Value bit mask. */
#define TCA_SPLIT_LCMP0OV_bp  0  /* Low Compare 0 Output Value bit position. */
#define TCA_SPLIT_LCMP1OV_bm  0x02  /* Low Compare 1 Output Value bit mask. */
#define TCA_SPLIT_LCMP1OV_bp  1  /* Low Compare 1 Output Value bit position. */
#define TCA_SPLIT_LCMP2OV_bm  0x04  /* Low Compare 2 Output Value bit mask. */
#define TCA_SPLIT_LCMP2OV_bp  2  /* Low Compare 2 Output Value bit position. */
#define TCA_SPLIT_HCMP0OV_bm  0x10  /* High Compare 0 Output Value bit mask. */
#define TCA_SPLIT_HCMP0OV_bp  4  /* High Compare 0 Output Value bit position. */
#define TCA_SPLIT_HCMP1OV_bm  0x20  /* High Compare 1 Output Value bit mask. */
#define TCA_SPLIT_HCMP1OV_bp  5  /* High Compare 1 Output Value bit position. */
#define TCA_SPLIT_HCMP2OV_bm  0x40  /* High Compare 2 Output Value bit mask. */
#define TCA_SPLIT_HCMP2OV_bp  6  /* High Compare 2 Output Value bit position. */

/* TCA_SPLIT.CTRLD  bit masks and bit positions */
#define TCA_SPLIT_SPLITM_bm  0x01  /* Split Mode Enable bit mask. */
#define TCA_SPLIT_SPLITM_bp  0  /* Split Mode Enable bit position. */

/* TCA_SPLIT.CTRLECLR  bit masks and bit positions */
#define TCA_SPLIT_CMDEN_gm  0x03  /* Command Enable group mask. */
#define TCA_SPLIT_CMDEN_gp  0  /* Command Enable group position. */
#define TCA_SPLIT_CMDEN_0_bm  (1<<0)  /* Command Enable bit 0 mask. */
#define TCA_SPLIT_CMDEN_0_bp  0  /* Command Enable bit 0 position. */
#define TCA_SPLIT_CMDEN_1_bm  (1<<1)  /* Command Enable bit 1 mask. */
#define TCA_SPLIT_CMDEN_1_bp  1  /* Command Enable bit 1 position. */
#define TCA_SPLIT_CMD_gm  0x0C  /* Command group mask. */
#define TCA_SPLIT_CMD_gp  2  /* Command group position. */
#define TCA_SPLIT_CMD_0_bm  (1<<2)  /* Command bit 0 mask. */
#define TCA_SPLIT_CMD_0_bp  2  /* Command bit 0 position. */
#define TCA_SPLIT_CMD_1_bm  (1<<3)  /* Command bit 1 mask. */
#define TCA_SPLIT_CMD_1_bp  3  /* Command bit 1 position. */

/* TCA_SPLIT.CTRLESET  bit masks and bit positions */
/* TCA_SPLIT_CMDEN  is already defined. */
/* TCA_SPLIT_CMD  is already defined. */

/* TCA_SPLIT.INTCTRL  bit masks and bit positions */
#define TCA_SPLIT_LUNF_bm  0x01  /* Low Underflow Interrupt Enable bit mask. */
#define TCA_SPLIT_LUNF_bp  0  /* Low Underflow Interrupt Enable bit position. */
#define TCA_SPLIT_HUNF_bm  0x02  /* High Underflow Interrupt Enable bit mask. */
#define TCA_SPLIT_HUNF_bp  1  /* High Underflow Interrupt Enable bit position. */
#define TCA_SPLIT_LCMP0_bm  0x10  /* Low Compare 0 Interrupt Enable bit mask. */
#define TCA_SPLIT_LCMP0_bp  4  /* Low Compare 0 Interrupt Enable bit position. */
#define TCA_SPLIT_LCMP1_bm  0x20  /* Low Compare 1 Interrupt Enable bit mask. */
#define TCA_SPLIT_LCMP1_bp  5  /* Low Compare 1 Interrupt Enable bit position. */
#define TCA_SPLIT_LCMP2_bm  0x40  /* Low Compare 2 Interrupt Enable bit mask. */
#define TCA_SPLIT_LCMP2_bp  6  /* Low Compare 2 Interrupt Enable bit position. */

/* TCA_SPLIT.INTFLAGS  bit masks and bit positions */
/* TCA_SPLIT_LUNF  is already defined. */
/* TCA_SPLIT_HUNF  is already defined. */
/* TCA_SPLIT_LCMP0  is already defined. */
/* TCA_SPLIT_LCMP1  is already defined. */
/* TCA_SPLIT_LCMP2  is already defined. */

/* TCA_SPLIT.DBGCTRL  bit masks and bit positions */
#define TCA_SPLIT_DBGRUN_bm  0x01  /* Debug Run bit mask. */
#define TCA_SPLIT_DBGRUN_bp  0  /* Debug Run bit position. */


/* 16-bit Timer/Counter Type A - Single Mode */
typedef struct TCA_SINGLE_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t CTRLC;  /* Control C */
    register8_t CTRLD;  /* Control D */
    register8_t CTRLECLR;  /* Control E Clear */
    register8_t CTRLESET;  /* Control E Set */
    register8_t CTRLFCLR;  /* Control F Clear */
    register8_t CTRLFSET;  /* Control F Set */
    register8_t reserved_1[1];
    register8_t EVCTRL;  /* Event Control */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t reserved_2[2];
    register8_t DBGCTRL;  /* Debug Control */
    register8_t TEMP;  /* Temporary data for 16-bit Access */
    register8_t reserved_3[16];
    _WORDREGISTER(CNT);  /* Count */
    register8_t reserved_4[4];
    _WORDREGISTER(PER);  /* Period */
    _WORDREGISTER(CMP0);  /* Compare 0 */
    _WORDREGISTER(CMP1);  /* Compare 1 */
    _WORDREGISTER(CMP2);  /* Compare 2 */
    register8_t reserved_5[8];
    _WORDREGISTER(PERBUF);  /* Period Buffer */
    _WORDREGISTER(CMP0BUF);  /* Compare 0 Buffer */
    _WORDREGISTER(CMP1BUF);  /* Compare 1 Buffer */
    _WORDREGISTER(CMP2BUF);  /* Compare 2 Buffer */
    register8_t reserved_6[2];
} TCA_SINGLE_t;

/* 16-bit Timer/Counter Type A - Split Mode */
typedef struct TCA_SPLIT_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t CTRLC;  /* Control C */
    register8_t CTRLD;  /* Control D */
    register8_t CTRLECLR;  /* Control E Clear */
    register8_t CTRLESET;  /* Control E Set */
    register8_t reserved_1[4];
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t reserved_2[2];
    register8_t DBGCTRL;  /* Debug Control */
    register8_t reserved_3[17];
    register8_t LCNT;  /* Low Count */
    register8_t HCNT;  /* High Count */
    register8_t reserved_4[4];
    register8_t LPER;  /* Low Period */
    register8_t HPER;  /* High Period */
    register8_t LCMP0;  /* Low Compare */
    register8_t HCMP0;  /* High Compare */
    register8_t LCMP1;  /* Low Compare */
    register8_t HCMP1;  /* High Compare */
    register8_t LCMP2;  /* Low Compare */
    register8_t HCMP2;  /* High Compare */
    register8_t reserved_5[18];
} TCA_SPLIT_t;

/* 16-bit Timer/Counter Type A */
typedef union TCA_union
{
    TCA_SINGLE_t SINGLE;  /* Single Mode */
    TCA_SPLIT_t SPLIT;  /* Split Mode */
} TCA_t;

/* Clock Selection bit group values */
typedef enum TCA_SINGLE_CLKSEL_VALUES_enum
{
    TCA_SINGLE_CLKSEL_DIV1_gv      = 0x00,  /* CLK_PER */
    TCA_SINGLE_CLKSEL_DIV2_gv      = 0x01,  /* CLK_PER / 2 */
    TCA_SINGLE_CLKSEL_DIV4_gv      = 0x02,  /* CLK_PER / 4 */
    TCA_SINGLE_CLKSEL_DIV8_gv      = 0x03,  /* CLK_PER / 8 */
    TCA_SINGLE_CLKSEL_DIV16_gv     = 0x04,  /* CLK_PER / 16 */
    TCA_SINGLE_CLKSEL_DIV64_gv     = 0x05,  /* CLK_PER / 64 */
    TCA_SINGLE_CLKSEL_DIV256_gv    = 0x06,  /* CLK_PER / 256 */
    TCA_SINGLE_CLKSEL_DIV1024_gv   = 0x07   /* CLK_PER / 1024 */
} TCA_SINGLE_CLKSEL_values_t;

/* Clock Selection bit group configurations*/
typedef enum TCA_SINGLE_CLKSEL_enum
{
    TCA_SINGLE_CLKSEL_DIV1_gc      = (TCA_SINGLE_CLKSEL_DIV1_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER */
    TCA_SINGLE_CLKSEL_DIV2_gc      = (TCA_SINGLE_CLKSEL_DIV2_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER / 2 */
    TCA_SINGLE_CLKSEL_DIV4_gc      = (TCA_SINGLE_CLKSEL_DIV4_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER / 4 */
    TCA_SINGLE_CLKSEL_DIV8_gc      = (TCA_SINGLE_CLKSEL_DIV8_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER / 8 */
    TCA_SINGLE_CLKSEL_DIV16_gc     = (TCA_SINGLE_CLKSEL_DIV16_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER / 16 */
    TCA_SINGLE_CLKSEL_DIV64_gc     = (TCA_SINGLE_CLKSEL_DIV64_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER / 64 */
    TCA_SINGLE_CLKSEL_DIV256_gc    = (TCA_SINGLE_CLKSEL_DIV256_gv << TCA_SINGLE_CLKSEL_gp),  /* CLK_PER / 256 */
    TCA_SINGLE_CLKSEL_DIV1024_gc   = (TCA_SINGLE_CLKSEL_DIV1024_gv << TCA_SINGLE_CLKSEL_gp)   /* CLK_PER / 1024 */
} TCA_SINGLE_CLKSEL_t;

/* Command select bit group values */
typedef enum TCA_SINGLE_CMD_VALUES_enum
{
    TCA_SINGLE_CMD_NONE_gv         = 0x00,  /* No Command */
    TCA_SINGLE_CMD_UPDATE_gv       = 0x01,  /* Force Update */
    TCA_SINGLE_CMD_RESTART_gv      = 0x02,  /* Force Restart */
    TCA_SINGLE_CMD_RESET_gv        = 0x03   /* Force Hard Reset */
} TCA_SINGLE_CMD_values_t;

/* Command select bit group configurations*/
typedef enum TCA_SINGLE_CMD_enum
{
    TCA_SINGLE_CMD_NONE_gc         = (TCA_SINGLE_CMD_NONE_gv << TCA_SINGLE_CMD_gp),  /* No Command */
    TCA_SINGLE_CMD_UPDATE_gc       = (TCA_SINGLE_CMD_UPDATE_gv << TCA_SINGLE_CMD_gp),  /* Force Update */
    TCA_SINGLE_CMD_RESTART_gc      = (TCA_SINGLE_CMD_RESTART_gv << TCA_SINGLE_CMD_gp),  /* Force Restart */
    TCA_SINGLE_CMD_RESET_gc        = (TCA_SINGLE_CMD_RESET_gv << TCA_SINGLE_CMD_gp)   /* Force Hard Reset */
} TCA_SINGLE_CMD_t;

/* Direction select bit group values */
typedef enum TCA_SINGLE_DIR_VALUES_enum
{
    TCA_SINGLE_DIR_UP_gv           = 0x00,  /* Count up */
    TCA_SINGLE_DIR_DOWN_gv         = 0x01   /* Count down */
} TCA_SINGLE_DIR_values_t;

/* Direction select bit group configurations*/
typedef enum TCA_SINGLE_DIR_enum
{
    TCA_SINGLE_DIR_UP_gc           = (TCA_SINGLE_DIR_UP_gv << TCA_SINGLE_DIR_bp),  /* Count up */
    TCA_SINGLE_DIR_DOWN_gc         = (TCA_SINGLE_DIR_DOWN_gv << TCA_SINGLE_DIR_bp)   /* Count down */
} TCA_SINGLE_DIR_t;

/* Event Action A select bit group values */
typedef enum TCA_SINGLE_EVACTA_VALUES_enum
{
    TCA_SINGLE_EVACTA_CNT_POSEDGE_gv = 0x00,  /* Count on positive edge event */
    TCA_SINGLE_EVACTA_CNT_ANYEDGE_gv = 0x01,  /* Count on any edge event */
    TCA_SINGLE_EVACTA_CNT_HIGHLVL_gv = 0x02,  /* Count on prescaled clock while event line is 1. */
    TCA_SINGLE_EVACTA_UPDOWN_gv    = 0x03   /* Count on prescaled clock. Event controls count direction. Up-count when event line is 0, down-count when event line is 1. */
} TCA_SINGLE_EVACTA_values_t;

/* Event Action A select bit group configurations*/
typedef enum TCA_SINGLE_EVACTA_enum
{
    TCA_SINGLE_EVACTA_CNT_POSEDGE_gc = (TCA_SINGLE_EVACTA_CNT_POSEDGE_gv << TCA_SINGLE_EVACTA_gp),  /* Count on positive edge event */
    TCA_SINGLE_EVACTA_CNT_ANYEDGE_gc = (TCA_SINGLE_EVACTA_CNT_ANYEDGE_gv << TCA_SINGLE_EVACTA_gp),  /* Count on any edge event */
    TCA_SINGLE_EVACTA_CNT_HIGHLVL_gc = (TCA_SINGLE_EVACTA_CNT_HIGHLVL_gv << TCA_SINGLE_EVACTA_gp),  /* Count on prescaled clock while event line is 1. */
    TCA_SINGLE_EVACTA_UPDOWN_gc    = (TCA_SINGLE_EVACTA_UPDOWN_gv << TCA_SINGLE_EVACTA_gp)   /* Count on prescaled clock. Event controls count direction. Up-count when event line is 0, down-count when event line is 1. */
} TCA_SINGLE_EVACTA_t;

/* Event Action B select bit group values */
typedef enum TCA_SINGLE_EVACTB_VALUES_enum
{
    TCA_SINGLE_EVACTB_NONE_gv      = 0x00,  /* No Action */
    TCA_SINGLE_EVACTB_UPDOWN_gv    = 0x03,  /* Count on prescaled clock. Event controls count direction. Up-count when event line is 0, down-count when event line is 1. */
    TCA_SINGLE_EVACTB_RESTART_POSEDGE_gv = 0x04,  /* Restart counter at positive edge event */
    TCA_SINGLE_EVACTB_RESTART_ANYEDGE_gv = 0x05,  /* Restart counter on any edge event */
    TCA_SINGLE_EVACTB_RESTART_HIGHLVL_gv = 0x06   /* Restart counter while event line is 1. */
} TCA_SINGLE_EVACTB_values_t;

/* Event Action B select bit group configurations*/
typedef enum TCA_SINGLE_EVACTB_enum
{
    TCA_SINGLE_EVACTB_NONE_gc      = (TCA_SINGLE_EVACTB_NONE_gv << TCA_SINGLE_EVACTB_gp),  /* No Action */
    TCA_SINGLE_EVACTB_UPDOWN_gc    = (TCA_SINGLE_EVACTB_UPDOWN_gv << TCA_SINGLE_EVACTB_gp),  /* Count on prescaled clock. Event controls count direction. Up-count when event line is 0, down-count when event line is 1. */
    TCA_SINGLE_EVACTB_RESTART_POSEDGE_gc = (TCA_SINGLE_EVACTB_RESTART_POSEDGE_gv << TCA_SINGLE_EVACTB_gp),  /* Restart counter at positive edge event */
    TCA_SINGLE_EVACTB_RESTART_ANYEDGE_gc = (TCA_SINGLE_EVACTB_RESTART_ANYEDGE_gv << TCA_SINGLE_EVACTB_gp),  /* Restart counter on any edge event */
    TCA_SINGLE_EVACTB_RESTART_HIGHLVL_gc = (TCA_SINGLE_EVACTB_RESTART_HIGHLVL_gv << TCA_SINGLE_EVACTB_gp)   /* Restart counter while event line is 1. */
} TCA_SINGLE_EVACTB_t;

/* Waveform generation mode select bit group values */
typedef enum TCA_SINGLE_WGMODE_VALUES_enum
{
    TCA_SINGLE_WGMODE_NORMAL_gv    = 0x00,  /* Normal Mode */
    TCA_SINGLE_WGMODE_FRQ_gv       = 0x01,  /* Frequency Generation Mode */
    TCA_SINGLE_WGMODE_SINGLESLOPE_gv = 0x03,  /* Single Slope PWM */
    TCA_SINGLE_WGMODE_DSTOP_gv     = 0x05,  /* Dual Slope PWM, overflow on TOP */
    TCA_SINGLE_WGMODE_DSBOTH_gv    = 0x06,  /* Dual Slope PWM, overflow on TOP and BOTTOM */
    TCA_SINGLE_WGMODE_DSBOTTOM_gv  = 0x07   /* Dual Slope PWM, overflow on BOTTOM */
} TCA_SINGLE_WGMODE_values_t;

/* Waveform generation mode select bit group configurations*/
typedef enum TCA_SINGLE_WGMODE_enum
{
    TCA_SINGLE_WGMODE_NORMAL_gc    = (TCA_SINGLE_WGMODE_NORMAL_gv << TCA_SINGLE_WGMODE_gp),  /* Normal Mode */
    TCA_SINGLE_WGMODE_FRQ_gc       = (TCA_SINGLE_WGMODE_FRQ_gv << TCA_SINGLE_WGMODE_gp),  /* Frequency Generation Mode */
    TCA_SINGLE_WGMODE_SINGLESLOPE_gc = (TCA_SINGLE_WGMODE_SINGLESLOPE_gv << TCA_SINGLE_WGMODE_gp),  /* Single Slope PWM */
    TCA_SINGLE_WGMODE_DSTOP_gc     = (TCA_SINGLE_WGMODE_DSTOP_gv << TCA_SINGLE_WGMODE_gp),  /* Dual Slope PWM, overflow on TOP */
    TCA_SINGLE_WGMODE_DSBOTH_gc    = (TCA_SINGLE_WGMODE_DSBOTH_gv << TCA_SINGLE_WGMODE_gp),  /* Dual Slope PWM, overflow on TOP and BOTTOM */
    TCA_SINGLE_WGMODE_DSBOTTOM_gc  = (TCA_SINGLE_WGMODE_DSBOTTOM_gv << TCA_SINGLE_WGMODE_gp)   /* Dual Slope PWM, overflow on BOTTOM */
} TCA_SINGLE_WGMODE_t;

/* Clock Selection bit group values */
typedef enum TCA_SPLIT_CLKSEL_VALUES_enum
{
    TCA_SPLIT_CLKSEL_DIV1_gv       = 0x00,  /* CLK_PER */
    TCA_SPLIT_CLKSEL_DIV2_gv       = 0x01,  /* CLK_PER / 2 */
    TCA_SPLIT_CLKSEL_DIV4_gv       = 0x02,  /* CLK_PER / 4 */
    TCA_SPLIT_CLKSEL_DIV8_gv       = 0x03,  /* CLK_PER / 8 */
    TCA_SPLIT_CLKSEL_DIV16_gv      = 0x04,  /* CLK_PER / 16 */
    TCA_SPLIT_CLKSEL_DIV64_gv      = 0x05,  /* CLK_PER / 64 */
    TCA_SPLIT_CLKSEL_DIV256_gv     = 0x06,  /* CLK_PER / 256 */
    TCA_SPLIT_CLKSEL_DIV1024_gv    = 0x07   /* CLK_PER / 1024 */
} TCA_SPLIT_CLKSEL_values_t;

/* Clock Selection bit group configurations*/
typedef enum TCA_SPLIT_CLKSEL_enum
{
    TCA_SPLIT_CLKSEL_DIV1_gc       = (TCA_SPLIT_CLKSEL_DIV1_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER */
    TCA_SPLIT_CLKSEL_DIV2_gc       = (TCA_SPLIT_CLKSEL_DIV2_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER / 2 */
    TCA_SPLIT_CLKSEL_DIV4_gc       = (TCA_SPLIT_CLKSEL_DIV4_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER / 4 */
    TCA_SPLIT_CLKSEL_DIV8_gc       = (TCA_SPLIT_CLKSEL_DIV8_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER / 8 */
    TCA_SPLIT_CLKSEL_DIV16_gc      = (TCA_SPLIT_CLKSEL_DIV16_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER / 16 */
    TCA_SPLIT_CLKSEL_DIV64_gc      = (TCA_SPLIT_CLKSEL_DIV64_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER / 64 */
    TCA_SPLIT_CLKSEL_DIV256_gc     = (TCA_SPLIT_CLKSEL_DIV256_gv << TCA_SPLIT_CLKSEL_gp),  /* CLK_PER / 256 */
    TCA_SPLIT_CLKSEL_DIV1024_gc    = (TCA_SPLIT_CLKSEL_DIV1024_gv << TCA_SPLIT_CLKSEL_gp)   /* CLK_PER / 1024 */
} TCA_SPLIT_CLKSEL_t;

/* Command select bit group values */
typedef enum TCA_SPLIT_CMD_VALUES_enum
{
    TCA_SPLIT_CMD_NONE_gv          = 0x00,  /* No Command */
    TCA_SPLIT_CMD_UPDATE_gv        = 0x01,  /* Force Update */
    TCA_SPLIT_CMD_RESTART_gv       = 0x02,  /* Force Restart */
    TCA_SPLIT_CMD_RESET_gv         = 0x03   /* Force Hard Reset */
} TCA_SPLIT_CMD_values_t;

/* Command select bit group configurations*/
typedef enum TCA_SPLIT_CMD_enum
{
    TCA_SPLIT_CMD_NONE_gc          = (TCA_SPLIT_CMD_NONE_gv << TCA_SPLIT_CMD_gp),  /* No Command */
    TCA_SPLIT_CMD_UPDATE_gc        = (TCA_SPLIT_CMD_UPDATE_gv << TCA_SPLIT_CMD_gp),  /* Force Update */
    TCA_SPLIT_CMD_RESTART_gc       = (TCA_SPLIT_CMD_RESTART_gv << TCA_SPLIT_CMD_gp),  /* Force Restart */
    TCA_SPLIT_CMD_RESET_gc         = (TCA_SPLIT_CMD_RESET_gv << TCA_SPLIT_CMD_gp)   /* Force Hard Reset */
} TCA_SPLIT_CMD_t;

/* Command Enable select bit group values */
typedef enum TCA_SPLIT_CMDEN_VALUES_enum
{
    TCA_SPLIT_CMDEN_NONE_gv        = 0x00,  /* None */
    TCA_SPLIT_CMDEN_BOTH_gv        = 0x03   /* Both low byte and high byte counter */
} TCA_SPLIT_CMDEN_values_t;

/* Command Enable select bit group configurations*/
typedef enum TCA_SPLIT_CMDEN_enum
{
    TCA_SPLIT_CMDEN_NONE_gc        = (TCA_SPLIT_CMDEN_NONE_gv << TCA_SPLIT_CMDEN_gp),  /* None */
    TCA_SPLIT_CMDEN_BOTH_gc        = (TCA_SPLIT_CMDEN_BOTH_gv << TCA_SPLIT_CMDEN_gp)   /* Both low byte and high byte counter */
} TCA_SPLIT_CMDEN_t;

/* End of extract */

#endif //_XT_IO_TCA_H_
