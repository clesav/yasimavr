/*
 * io_acp.h
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

#ifndef _XT_IO_ACP_H_
#define _XT_IO_ACP_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the AC peripheral for the following device models:
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


/* AC - Analog Comparator */
/* AC.CTRLA  bit masks and bit positions */
#define AC_ENABLE_bm  0x01  /* Enable bit mask. */
#define AC_ENABLE_bp  0  /* Enable bit position. */
#define AC_HYSMODE_gm  0x06  /* Hysteresis Mode group mask. */
#define AC_HYSMODE_gp  1  /* Hysteresis Mode group position. */
#define AC_HYSMODE_0_bm  (1<<1)  /* Hysteresis Mode bit 0 mask. */
#define AC_HYSMODE_0_bp  1  /* Hysteresis Mode bit 0 position. */
#define AC_HYSMODE_1_bm  (1<<2)  /* Hysteresis Mode bit 1 mask. */
#define AC_HYSMODE_1_bp  2  /* Hysteresis Mode bit 1 position. */
#define AC_LPMODE_bm  0x08  /* Low Power Mode bit mask. */
#define AC_LPMODE_bp  3  /* Low Power Mode bit position. */
#define AC_INTMODE_gm  0x30  /* Interrupt Mode group mask. */
#define AC_INTMODE_gp  4  /* Interrupt Mode group position. */
#define AC_INTMODE_0_bm  (1<<4)  /* Interrupt Mode bit 0 mask. */
#define AC_INTMODE_0_bp  4  /* Interrupt Mode bit 0 position. */
#define AC_INTMODE_1_bm  (1<<5)  /* Interrupt Mode bit 1 mask. */
#define AC_INTMODE_1_bp  5  /* Interrupt Mode bit 1 position. */
#define AC_OUTEN_bm  0x40  /* Output Buffer Enable bit mask. */
#define AC_OUTEN_bp  6  /* Output Buffer Enable bit position. */
#define AC_RUNSTDBY_bm  0x80  /* Run in Standby Mode bit mask. */
#define AC_RUNSTDBY_bp  7  /* Run in Standby Mode bit position. */

/* AC.MUXCTRLA  bit masks and bit positions */
#define AC_MUXNEG_gm  0x03  /* Negative Input MUX Selection group mask. */
#define AC_MUXNEG_gp  0  /* Negative Input MUX Selection group position. */
#define AC_MUXNEG_0_bm  (1<<0)  /* Negative Input MUX Selection bit 0 mask. */
#define AC_MUXNEG_0_bp  0  /* Negative Input MUX Selection bit 0 position. */
#define AC_MUXNEG_1_bm  (1<<1)  /* Negative Input MUX Selection bit 1 mask. */
#define AC_MUXNEG_1_bp  1  /* Negative Input MUX Selection bit 1 position. */
#define AC_MUXPOS_gm  0x18  /* Positive Input MUX Selection group mask. */
#define AC_MUXPOS_gp  3  /* Positive Input MUX Selection group position. */
#define AC_MUXPOS_0_bm  (1<<3)  /* Positive Input MUX Selection bit 0 mask. */
#define AC_MUXPOS_0_bp  3  /* Positive Input MUX Selection bit 0 position. */
#define AC_MUXPOS_1_bm  (1<<4)  /* Positive Input MUX Selection bit 1 mask. */
#define AC_MUXPOS_1_bp  4  /* Positive Input MUX Selection bit 1 position. */
#define AC_INVERT_bm  0x80  /* Invert AC Output bit mask. */
#define AC_INVERT_bp  7  /* Invert AC Output bit position. */

/* AC.DACREF  bit masks and bit positions */
#define AC_DACREF_gm  0xFF  /* DACREF Data Value group mask. */
#define AC_DACREF_gp  0  /* DACREF Data Value group position. */
#define AC_DACREF_0_bm  (1<<0)  /* DACREF Data Value bit 0 mask. */
#define AC_DACREF_0_bp  0  /* DACREF Data Value bit 0 position. */
#define AC_DACREF_1_bm  (1<<1)  /* DACREF Data Value bit 1 mask. */
#define AC_DACREF_1_bp  1  /* DACREF Data Value bit 1 position. */
#define AC_DACREF_2_bm  (1<<2)  /* DACREF Data Value bit 2 mask. */
#define AC_DACREF_2_bp  2  /* DACREF Data Value bit 2 position. */
#define AC_DACREF_3_bm  (1<<3)  /* DACREF Data Value bit 3 mask. */
#define AC_DACREF_3_bp  3  /* DACREF Data Value bit 3 position. */
#define AC_DACREF_4_bm  (1<<4)  /* DACREF Data Value bit 4 mask. */
#define AC_DACREF_4_bp  4  /* DACREF Data Value bit 4 position. */
#define AC_DACREF_5_bm  (1<<5)  /* DACREF Data Value bit 5 mask. */
#define AC_DACREF_5_bp  5  /* DACREF Data Value bit 5 position. */
#define AC_DACREF_6_bm  (1<<6)  /* DACREF Data Value bit 6 mask. */
#define AC_DACREF_6_bp  6  /* DACREF Data Value bit 6 position. */
#define AC_DACREF_7_bm  (1<<7)  /* DACREF Data Value bit 7 mask. */
#define AC_DACREF_7_bp  7  /* DACREF Data Value bit 7 position. */

/* AC.INTCTRL  bit masks and bit positions */
#define AC_CMP_bm  0x01  /* Analog Comparator 0 Interrupt Enable bit mask. */
#define AC_CMP_bp  0  /* Analog Comparator 0 Interrupt Enable bit position. */

/* AC.STATUS  bit masks and bit positions */
/* AC_CMP  is already defined. */
#define AC_STATE_bm  0x10  /* Analog Comparator State bit mask. */
#define AC_STATE_bp  4  /* Analog Comparator State bit position. */


/* Analog Comparator */
typedef struct AC_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t reserved_1[1];
    register8_t MUXCTRLA;  /* Mux Control A */
    register8_t reserved_2[1];
    register8_t DACREF;  /* Referance scale control */
    register8_t reserved_3[1];
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t STATUS;  /* Status */
} AC_t;

/* Hysteresis Mode select bit group values */
typedef enum AC_HYSMODE_VALUES_enum
{
    AC_HYSMODE_OFF_gv              = 0x00,  /* No hysteresis */
    AC_HYSMODE_10mV_gv             = 0x01,  /* 10mV hysteresis */
    AC_HYSMODE_25mV_gv             = 0x02,  /* 25mV hysteresis */
    AC_HYSMODE_50mV_gv             = 0x03   /* 50mV hysteresis */
} AC_HYSMODE_values_t;

/* Hysteresis Mode select bit group configurations*/
typedef enum AC_HYSMODE_enum
{
    AC_HYSMODE_OFF_gc              = (AC_HYSMODE_OFF_gv << AC_HYSMODE_gp),  /* No hysteresis */
    AC_HYSMODE_10mV_gc             = (AC_HYSMODE_10mV_gv << AC_HYSMODE_gp),  /* 10mV hysteresis */
    AC_HYSMODE_25mV_gc             = (AC_HYSMODE_25mV_gv << AC_HYSMODE_gp),  /* 25mV hysteresis */
    AC_HYSMODE_50mV_gc             = (AC_HYSMODE_50mV_gv << AC_HYSMODE_gp)   /* 50mV hysteresis */
} AC_HYSMODE_t;

/* Interrupt Mode select bit group values */
typedef enum AC_INTMODE_VALUES_enum
{
    AC_INTMODE_BOTHEDGE_gv         = 0x00,  /* Both Edge */
    AC_INTMODE_NEGEDGE_gv          = 0x02,  /* Negative Edge */
    AC_INTMODE_POSEDGE_gv          = 0x03   /* Positive Edge */
} AC_INTMODE_values_t;

/* Interrupt Mode select bit group configurations*/
typedef enum AC_INTMODE_enum
{
    AC_INTMODE_BOTHEDGE_gc         = (AC_INTMODE_BOTHEDGE_gv << AC_INTMODE_gp),  /* Both Edge */
    AC_INTMODE_NEGEDGE_gc          = (AC_INTMODE_NEGEDGE_gv << AC_INTMODE_gp),  /* Negative Edge */
    AC_INTMODE_POSEDGE_gc          = (AC_INTMODE_POSEDGE_gv << AC_INTMODE_gp)   /* Positive Edge */
} AC_INTMODE_t;

/* Low Power Mode select bit group values */
typedef enum AC_LPMODE_VALUES_enum
{
    AC_LPMODE_DIS_gv               = 0x00,  /* Low power mode disabled */
    AC_LPMODE_EN_gv                = 0x01   /* Low power mode enabled */
} AC_LPMODE_values_t;

/* Low Power Mode select bit group configurations*/
typedef enum AC_LPMODE_enum
{
    AC_LPMODE_DIS_gc               = (AC_LPMODE_DIS_gv << AC_LPMODE_bp),  /* Low power mode disabled */
    AC_LPMODE_EN_gc                = (AC_LPMODE_EN_gv << AC_LPMODE_bp)   /* Low power mode enabled */
} AC_LPMODE_t;

/* Negative Input MUX Selection bit group values */
typedef enum AC_MUXNEG_VALUES_enum
{
    AC_MUXNEG_AINN0_gv             = 0x00,  /* Negative Pin 0 */
    AC_MUXNEG_AINN1_gv             = 0x01,  /* Negative Pin 1 */
    AC_MUXNEG_AINN2_gv             = 0x02,  /* Negative Pin 2 */
    AC_MUXNEG_DACREF_gv            = 0x03   /* DAC Voltage Reference */
} AC_MUXNEG_values_t;

/* Negative Input MUX Selection bit group configurations*/
typedef enum AC_MUXNEG_enum
{
    AC_MUXNEG_AINN0_gc             = (AC_MUXNEG_AINN0_gv << AC_MUXNEG_gp),  /* Negative Pin 0 */
    AC_MUXNEG_AINN1_gc             = (AC_MUXNEG_AINN1_gv << AC_MUXNEG_gp),  /* Negative Pin 1 */
    AC_MUXNEG_AINN2_gc             = (AC_MUXNEG_AINN2_gv << AC_MUXNEG_gp),  /* Negative Pin 2 */
    AC_MUXNEG_DACREF_gc            = (AC_MUXNEG_DACREF_gv << AC_MUXNEG_gp)   /* DAC Voltage Reference */
} AC_MUXNEG_t;

/* Positive Input MUX Selection bit group values */
typedef enum AC_MUXPOS_VALUES_enum
{
    AC_MUXPOS_AINP0_gv             = 0x00,  /* Positive Pin 0 */
    AC_MUXPOS_AINP1_gv             = 0x01,  /* Positive Pin 1 */
    AC_MUXPOS_AINP2_gv             = 0x02,  /* Positive Pin 2 */
    AC_MUXPOS_AINP3_gv             = 0x03   /* Positive Pin 3 */
} AC_MUXPOS_values_t;

/* Positive Input MUX Selection bit group configurations*/
typedef enum AC_MUXPOS_enum
{
    AC_MUXPOS_AINP0_gc             = (AC_MUXPOS_AINP0_gv << AC_MUXPOS_gp),  /* Positive Pin 0 */
    AC_MUXPOS_AINP1_gc             = (AC_MUXPOS_AINP1_gv << AC_MUXPOS_gp),  /* Positive Pin 1 */
    AC_MUXPOS_AINP2_gc             = (AC_MUXPOS_AINP2_gv << AC_MUXPOS_gp),  /* Positive Pin 2 */
    AC_MUXPOS_AINP3_gc             = (AC_MUXPOS_AINP3_gv << AC_MUXPOS_gp)   /* Positive Pin 3 */
} AC_MUXPOS_t;

/* End of extract */

#endif //_XT_IO_ACP_H_
