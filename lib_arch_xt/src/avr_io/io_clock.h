/*
 * io_clock.h
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

#ifndef _XT_IO_CLOCK_H_
#define _XT_IO_CLOCK_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the CLKCTRL peripheral for the following device models:
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


/* CLKCTRL - Clock controller */
/* CLKCTRL.MCLKCTRLA  bit masks and bit positions */
#define CLKCTRL_CLKSEL_gm  0x03  /* Clock select group mask. */
#define CLKCTRL_CLKSEL_gp  0  /* Clock select group position. */
#define CLKCTRL_CLKSEL_0_bm  (1<<0)  /* Clock select bit 0 mask. */
#define CLKCTRL_CLKSEL_0_bp  0  /* Clock select bit 0 position. */
#define CLKCTRL_CLKSEL_1_bm  (1<<1)  /* Clock select bit 1 mask. */
#define CLKCTRL_CLKSEL_1_bp  1  /* Clock select bit 1 position. */
#define CLKCTRL_CLKOUT_bm  0x80  /* System clock out bit mask. */
#define CLKCTRL_CLKOUT_bp  7  /* System clock out bit position. */

/* CLKCTRL.MCLKCTRLB  bit masks and bit positions */
#define CLKCTRL_PEN_bm  0x01  /* Prescaler enable bit mask. */
#define CLKCTRL_PEN_bp  0  /* Prescaler enable bit position. */
#define CLKCTRL_PDIV_gm  0x1E  /* Prescaler division group mask. */
#define CLKCTRL_PDIV_gp  1  /* Prescaler division group position. */
#define CLKCTRL_PDIV_0_bm  (1<<1)  /* Prescaler division bit 0 mask. */
#define CLKCTRL_PDIV_0_bp  1  /* Prescaler division bit 0 position. */
#define CLKCTRL_PDIV_1_bm  (1<<2)  /* Prescaler division bit 1 mask. */
#define CLKCTRL_PDIV_1_bp  2  /* Prescaler division bit 1 position. */
#define CLKCTRL_PDIV_2_bm  (1<<3)  /* Prescaler division bit 2 mask. */
#define CLKCTRL_PDIV_2_bp  3  /* Prescaler division bit 2 position. */
#define CLKCTRL_PDIV_3_bm  (1<<4)  /* Prescaler division bit 3 mask. */
#define CLKCTRL_PDIV_3_bp  4  /* Prescaler division bit 3 position. */

/* CLKCTRL.MCLKLOCK  bit masks and bit positions */
#define CLKCTRL_LOCKEN_bm  0x01  /* Lock enable bit mask. */
#define CLKCTRL_LOCKEN_bp  0  /* Lock enable bit position. */

/* CLKCTRL.MCLKSTATUS  bit masks and bit positions */
#define CLKCTRL_SOSC_bm  0x01  /* System Oscillator changing bit mask. */
#define CLKCTRL_SOSC_bp  0  /* System Oscillator changing bit position. */
#define CLKCTRL_OSC20MS_bm  0x10  /* 20MHz oscillator status bit mask. */
#define CLKCTRL_OSC20MS_bp  4  /* 20MHz oscillator status bit position. */
#define CLKCTRL_OSC32KS_bm  0x20  /* 32KHz oscillator status bit mask. */
#define CLKCTRL_OSC32KS_bp  5  /* 32KHz oscillator status bit position. */
#define CLKCTRL_XOSC32KS_bm  0x40  /* 32.768 kHz Crystal Oscillator status bit mask. */
#define CLKCTRL_XOSC32KS_bp  6  /* 32.768 kHz Crystal Oscillator status bit position. */
#define CLKCTRL_EXTS_bm  0x80  /* External Clock status bit mask. */
#define CLKCTRL_EXTS_bp  7  /* External Clock status bit position. */

/* CLKCTRL.OSC20MCTRLA  bit masks and bit positions */
#define CLKCTRL_RUNSTDBY_bm  0x02  /* Run standby bit mask. */
#define CLKCTRL_RUNSTDBY_bp  1  /* Run standby bit position. */

/* CLKCTRL.OSC20MCALIBA  bit masks and bit positions */
#define CLKCTRL_CAL20M_gm  0x7F  /* Calibration group mask. */
#define CLKCTRL_CAL20M_gp  0  /* Calibration group position. */
#define CLKCTRL_CAL20M_0_bm  (1<<0)  /* Calibration bit 0 mask. */
#define CLKCTRL_CAL20M_0_bp  0  /* Calibration bit 0 position. */
#define CLKCTRL_CAL20M_1_bm  (1<<1)  /* Calibration bit 1 mask. */
#define CLKCTRL_CAL20M_1_bp  1  /* Calibration bit 1 position. */
#define CLKCTRL_CAL20M_2_bm  (1<<2)  /* Calibration bit 2 mask. */
#define CLKCTRL_CAL20M_2_bp  2  /* Calibration bit 2 position. */
#define CLKCTRL_CAL20M_3_bm  (1<<3)  /* Calibration bit 3 mask. */
#define CLKCTRL_CAL20M_3_bp  3  /* Calibration bit 3 position. */
#define CLKCTRL_CAL20M_4_bm  (1<<4)  /* Calibration bit 4 mask. */
#define CLKCTRL_CAL20M_4_bp  4  /* Calibration bit 4 position. */
#define CLKCTRL_CAL20M_5_bm  (1<<5)  /* Calibration bit 5 mask. */
#define CLKCTRL_CAL20M_5_bp  5  /* Calibration bit 5 position. */
#define CLKCTRL_CAL20M_6_bm  (1<<6)  /* Calibration bit 6 mask. */
#define CLKCTRL_CAL20M_6_bp  6  /* Calibration bit 6 position. */

/* CLKCTRL.OSC20MCALIBB  bit masks and bit positions */
#define CLKCTRL_TEMPCAL20M_gm  0x0F  /* Oscillator temperature coefficient group mask. */
#define CLKCTRL_TEMPCAL20M_gp  0  /* Oscillator temperature coefficient group position. */
#define CLKCTRL_TEMPCAL20M_0_bm  (1<<0)  /* Oscillator temperature coefficient bit 0 mask. */
#define CLKCTRL_TEMPCAL20M_0_bp  0  /* Oscillator temperature coefficient bit 0 position. */
#define CLKCTRL_TEMPCAL20M_1_bm  (1<<1)  /* Oscillator temperature coefficient bit 1 mask. */
#define CLKCTRL_TEMPCAL20M_1_bp  1  /* Oscillator temperature coefficient bit 1 position. */
#define CLKCTRL_TEMPCAL20M_2_bm  (1<<2)  /* Oscillator temperature coefficient bit 2 mask. */
#define CLKCTRL_TEMPCAL20M_2_bp  2  /* Oscillator temperature coefficient bit 2 position. */
#define CLKCTRL_TEMPCAL20M_3_bm  (1<<3)  /* Oscillator temperature coefficient bit 3 mask. */
#define CLKCTRL_TEMPCAL20M_3_bp  3  /* Oscillator temperature coefficient bit 3 position. */
#define CLKCTRL_LOCK_bm  0x80  /* Lock bit mask. */
#define CLKCTRL_LOCK_bp  7  /* Lock bit position. */

/* CLKCTRL.OSC32KCTRLA  bit masks and bit positions */
/* CLKCTRL_RUNSTDBY  is already defined. */

/* CLKCTRL.XOSC32KCTRLA  bit masks and bit positions */
#define CLKCTRL_ENABLE_bm  0x01  /* Enable bit mask. */
#define CLKCTRL_ENABLE_bp  0  /* Enable bit position. */
/* CLKCTRL_RUNSTDBY  is already defined. */
#define CLKCTRL_SEL_bm  0x04  /* Select bit mask. */
#define CLKCTRL_SEL_bp  2  /* Select bit position. */
#define CLKCTRL_CSUT_gm  0x30  /* Crystal startup time group mask. */
#define CLKCTRL_CSUT_gp  4  /* Crystal startup time group position. */
#define CLKCTRL_CSUT_0_bm  (1<<4)  /* Crystal startup time bit 0 mask. */
#define CLKCTRL_CSUT_0_bp  4  /* Crystal startup time bit 0 position. */
#define CLKCTRL_CSUT_1_bm  (1<<5)  /* Crystal startup time bit 1 mask. */
#define CLKCTRL_CSUT_1_bp  5  /* Crystal startup time bit 1 position. */


/* Clock controller */
typedef struct CLKCTRL_struct
{
    register8_t MCLKCTRLA;  /* MCLK Control A */
    register8_t MCLKCTRLB;  /* MCLK Control B */
    register8_t MCLKLOCK;  /* MCLK Lock */
    register8_t MCLKSTATUS;  /* MCLK Status */
    register8_t reserved_1[12];
    register8_t OSC20MCTRLA;  /* OSC20M Control A */
    register8_t OSC20MCALIBA;  /* OSC20M Calibration A */
    register8_t OSC20MCALIBB;  /* OSC20M Calibration B */
    register8_t reserved_2[5];
    register8_t OSC32KCTRLA;  /* OSC32K Control A */
    register8_t reserved_3[3];
    register8_t XOSC32KCTRLA;  /* XOSC32K Control A */
    register8_t reserved_4[3];
} CLKCTRL_t;

/* Clock select bit group values */
typedef enum CLKCTRL_CLKSEL_VALUES_enum
{
    CLKCTRL_CLKSEL_OSC20M_gv       = 0x00,  /* 16/20MHz internal oscillator */
    CLKCTRL_CLKSEL_OSCULP32K_gv    = 0x01,  /* 32.768kHz internal ultra low-power oscillator */
    CLKCTRL_CLKSEL_XOSC32K_gv      = 0x02,  /* 32.768kHz external crystal oscillator */
    CLKCTRL_CLKSEL_EXTCLK_gv       = 0x03   /* External clock */
} CLKCTRL_CLKSEL_values_t;

/* Clock select bit group configurations*/
typedef enum CLKCTRL_CLKSEL_enum
{
    CLKCTRL_CLKSEL_OSC20M_gc       = (CLKCTRL_CLKSEL_OSC20M_gv << CLKCTRL_CLKSEL_gp),  /* 16/20MHz internal oscillator */
    CLKCTRL_CLKSEL_OSCULP32K_gc    = (CLKCTRL_CLKSEL_OSCULP32K_gv << CLKCTRL_CLKSEL_gp),  /* 32.768kHz internal ultra low-power oscillator */
    CLKCTRL_CLKSEL_XOSC32K_gc      = (CLKCTRL_CLKSEL_XOSC32K_gv << CLKCTRL_CLKSEL_gp),  /* 32.768kHz external crystal oscillator */
    CLKCTRL_CLKSEL_EXTCLK_gc       = (CLKCTRL_CLKSEL_EXTCLK_gv << CLKCTRL_CLKSEL_gp)   /* External clock */
} CLKCTRL_CLKSEL_t;

/* Crystal startup time select bit group values */
typedef enum CLKCTRL_CSUT_VALUES_enum
{
    CLKCTRL_CSUT_1K_gv             = 0x00,  /* 1k cycles */
    CLKCTRL_CSUT_16K_gv            = 0x01,  /* 16k cycles */
    CLKCTRL_CSUT_32K_gv            = 0x02,  /* 32k cycles */
    CLKCTRL_CSUT_64K_gv            = 0x03   /* 64k cycles */
} CLKCTRL_CSUT_values_t;

/* Crystal startup time select bit group configurations*/
typedef enum CLKCTRL_CSUT_enum
{
    CLKCTRL_CSUT_1K_gc             = (CLKCTRL_CSUT_1K_gv << CLKCTRL_CSUT_gp),  /* 1k cycles */
    CLKCTRL_CSUT_16K_gc            = (CLKCTRL_CSUT_16K_gv << CLKCTRL_CSUT_gp),  /* 16k cycles */
    CLKCTRL_CSUT_32K_gc            = (CLKCTRL_CSUT_32K_gv << CLKCTRL_CSUT_gp),  /* 32k cycles */
    CLKCTRL_CSUT_64K_gc            = (CLKCTRL_CSUT_64K_gv << CLKCTRL_CSUT_gp)   /* 64k cycles */
} CLKCTRL_CSUT_t;

/* Prescaler division select bit group values */
typedef enum CLKCTRL_PDIV_VALUES_enum
{
    CLKCTRL_PDIV_2X_gv             = 0x00,  /* 2X */
    CLKCTRL_PDIV_4X_gv             = 0x01,  /* 4X */
    CLKCTRL_PDIV_8X_gv             = 0x02,  /* 8X */
    CLKCTRL_PDIV_16X_gv            = 0x03,  /* 16X */
    CLKCTRL_PDIV_32X_gv            = 0x04,  /* 32X */
    CLKCTRL_PDIV_64X_gv            = 0x05,  /* 64X */
    CLKCTRL_PDIV_6X_gv             = 0x08,  /* 6X */
    CLKCTRL_PDIV_10X_gv            = 0x09,  /* 10X */
    CLKCTRL_PDIV_12X_gv            = 0x0A,  /* 12X */
    CLKCTRL_PDIV_24X_gv            = 0x0B,  /* 24X */
    CLKCTRL_PDIV_48X_gv            = 0x0C   /* 48X */
} CLKCTRL_PDIV_values_t;

/* Prescaler division select bit group configurations*/
typedef enum CLKCTRL_PDIV_enum
{
    CLKCTRL_PDIV_2X_gc             = (CLKCTRL_PDIV_2X_gv << CLKCTRL_PDIV_gp),  /* 2X */
    CLKCTRL_PDIV_4X_gc             = (CLKCTRL_PDIV_4X_gv << CLKCTRL_PDIV_gp),  /* 4X */
    CLKCTRL_PDIV_8X_gc             = (CLKCTRL_PDIV_8X_gv << CLKCTRL_PDIV_gp),  /* 8X */
    CLKCTRL_PDIV_16X_gc            = (CLKCTRL_PDIV_16X_gv << CLKCTRL_PDIV_gp),  /* 16X */
    CLKCTRL_PDIV_32X_gc            = (CLKCTRL_PDIV_32X_gv << CLKCTRL_PDIV_gp),  /* 32X */
    CLKCTRL_PDIV_64X_gc            = (CLKCTRL_PDIV_64X_gv << CLKCTRL_PDIV_gp),  /* 64X */
    CLKCTRL_PDIV_6X_gc             = (CLKCTRL_PDIV_6X_gv << CLKCTRL_PDIV_gp),  /* 6X */
    CLKCTRL_PDIV_10X_gc            = (CLKCTRL_PDIV_10X_gv << CLKCTRL_PDIV_gp),  /* 10X */
    CLKCTRL_PDIV_12X_gc            = (CLKCTRL_PDIV_12X_gv << CLKCTRL_PDIV_gp),  /* 12X */
    CLKCTRL_PDIV_24X_gc            = (CLKCTRL_PDIV_24X_gv << CLKCTRL_PDIV_gp),  /* 24X */
    CLKCTRL_PDIV_48X_gc            = (CLKCTRL_PDIV_48X_gv << CLKCTRL_PDIV_gp)   /* 48X */
} CLKCTRL_PDIV_t;

/* End of extract */

#endif //_XT_IO_CLOCK_H_
