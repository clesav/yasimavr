/*
 * io_rtc.h
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

#ifndef _XT_IO_RTC_H_
#define _XT_IO_RTC_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the RTC peripheral for the following device models:
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


/* RTC - Real-Time Counter */
/* RTC.CTRLA  bit masks and bit positions */
#define RTC_RTCEN_bm  0x01  /* Enable bit mask. */
#define RTC_RTCEN_bp  0  /* Enable bit position. */
#define RTC_CORREN_bm  0x04  /* Correction enable bit mask. */
#define RTC_CORREN_bp  2  /* Correction enable bit position. */
#define RTC_PRESCALER_gm  0x78  /* Prescaling Factor group mask. */
#define RTC_PRESCALER_gp  3  /* Prescaling Factor group position. */
#define RTC_PRESCALER_0_bm  (1<<3)  /* Prescaling Factor bit 0 mask. */
#define RTC_PRESCALER_0_bp  3  /* Prescaling Factor bit 0 position. */
#define RTC_PRESCALER_1_bm  (1<<4)  /* Prescaling Factor bit 1 mask. */
#define RTC_PRESCALER_1_bp  4  /* Prescaling Factor bit 1 position. */
#define RTC_PRESCALER_2_bm  (1<<5)  /* Prescaling Factor bit 2 mask. */
#define RTC_PRESCALER_2_bp  5  /* Prescaling Factor bit 2 position. */
#define RTC_PRESCALER_3_bm  (1<<6)  /* Prescaling Factor bit 3 mask. */
#define RTC_PRESCALER_3_bp  6  /* Prescaling Factor bit 3 position. */
#define RTC_RUNSTDBY_bm  0x80  /* Run In Standby bit mask. */
#define RTC_RUNSTDBY_bp  7  /* Run In Standby bit position. */

/* RTC.STATUS  bit masks and bit positions */
#define RTC_CTRLABUSY_bm  0x01  /* CTRLA Synchronization Busy Flag bit mask. */
#define RTC_CTRLABUSY_bp  0  /* CTRLA Synchronization Busy Flag bit position. */
#define RTC_CNTBUSY_bm  0x02  /* Count Synchronization Busy Flag bit mask. */
#define RTC_CNTBUSY_bp  1  /* Count Synchronization Busy Flag bit position. */
#define RTC_PERBUSY_bm  0x04  /* Period Synchronization Busy Flag bit mask. */
#define RTC_PERBUSY_bp  2  /* Period Synchronization Busy Flag bit position. */
#define RTC_CMPBUSY_bm  0x08  /* Comparator Synchronization Busy Flag bit mask. */
#define RTC_CMPBUSY_bp  3  /* Comparator Synchronization Busy Flag bit position. */

/* RTC.INTCTRL  bit masks and bit positions */
#define RTC_OVF_bm  0x01  /* Overflow Interrupt enable bit mask. */
#define RTC_OVF_bp  0  /* Overflow Interrupt enable bit position. */
#define RTC_CMP_bm  0x02  /* Compare Match Interrupt enable bit mask. */
#define RTC_CMP_bp  1  /* Compare Match Interrupt enable bit position. */

/* RTC.INTFLAGS  bit masks and bit positions */
/* RTC_OVF  is already defined. */
/* RTC_CMP  is already defined. */

/* RTC.DBGCTRL  bit masks and bit positions */
#define RTC_DBGRUN_bm  0x01  /* Run in debug bit mask. */
#define RTC_DBGRUN_bp  0  /* Run in debug bit position. */

/* RTC.CALIB  bit masks and bit positions */
#define RTC_ERROR_gm  0x7F  /* Error Correction Value group mask. */
#define RTC_ERROR_gp  0  /* Error Correction Value group position. */
#define RTC_ERROR_0_bm  (1<<0)  /* Error Correction Value bit 0 mask. */
#define RTC_ERROR_0_bp  0  /* Error Correction Value bit 0 position. */
#define RTC_ERROR_1_bm  (1<<1)  /* Error Correction Value bit 1 mask. */
#define RTC_ERROR_1_bp  1  /* Error Correction Value bit 1 position. */
#define RTC_ERROR_2_bm  (1<<2)  /* Error Correction Value bit 2 mask. */
#define RTC_ERROR_2_bp  2  /* Error Correction Value bit 2 position. */
#define RTC_ERROR_3_bm  (1<<3)  /* Error Correction Value bit 3 mask. */
#define RTC_ERROR_3_bp  3  /* Error Correction Value bit 3 position. */
#define RTC_ERROR_4_bm  (1<<4)  /* Error Correction Value bit 4 mask. */
#define RTC_ERROR_4_bp  4  /* Error Correction Value bit 4 position. */
#define RTC_ERROR_5_bm  (1<<5)  /* Error Correction Value bit 5 mask. */
#define RTC_ERROR_5_bp  5  /* Error Correction Value bit 5 position. */
#define RTC_ERROR_6_bm  (1<<6)  /* Error Correction Value bit 6 mask. */
#define RTC_ERROR_6_bp  6  /* Error Correction Value bit 6 position. */
#define RTC_SIGN_bm  0x80  /* Error Correction Sign Bit bit mask. */
#define RTC_SIGN_bp  7  /* Error Correction Sign Bit bit position. */

/* RTC.CLKSEL  bit masks and bit positions */
#define RTC_CLKSEL_gm  0x03  /* Clock Select group mask. */
#define RTC_CLKSEL_gp  0  /* Clock Select group position. */
#define RTC_CLKSEL_0_bm  (1<<0)  /* Clock Select bit 0 mask. */
#define RTC_CLKSEL_0_bp  0  /* Clock Select bit 0 position. */
#define RTC_CLKSEL_1_bm  (1<<1)  /* Clock Select bit 1 mask. */
#define RTC_CLKSEL_1_bp  1  /* Clock Select bit 1 position. */

/* RTC.PITCTRLA  bit masks and bit positions */
#define RTC_PITEN_bm  0x01  /* Enable bit mask. */
#define RTC_PITEN_bp  0  /* Enable bit position. */
#define RTC_PERIOD_gm  0x78  /* Period group mask. */
#define RTC_PERIOD_gp  3  /* Period group position. */
#define RTC_PERIOD_0_bm  (1<<3)  /* Period bit 0 mask. */
#define RTC_PERIOD_0_bp  3  /* Period bit 0 position. */
#define RTC_PERIOD_1_bm  (1<<4)  /* Period bit 1 mask. */
#define RTC_PERIOD_1_bp  4  /* Period bit 1 position. */
#define RTC_PERIOD_2_bm  (1<<5)  /* Period bit 2 mask. */
#define RTC_PERIOD_2_bp  5  /* Period bit 2 position. */
#define RTC_PERIOD_3_bm  (1<<6)  /* Period bit 3 mask. */
#define RTC_PERIOD_3_bp  6  /* Period bit 3 position. */

/* RTC.PITSTATUS  bit masks and bit positions */
#define RTC_CTRLBUSY_bm  0x01  /* CTRLA Synchronization Busy Flag bit mask. */
#define RTC_CTRLBUSY_bp  0  /* CTRLA Synchronization Busy Flag bit position. */

/* RTC.PITINTCTRL  bit masks and bit positions */
#define RTC_PI_bm  0x01  /* Periodic Interrupt bit mask. */
#define RTC_PI_bp  0  /* Periodic Interrupt bit position. */

/* RTC.PITINTFLAGS  bit masks and bit positions */
/* RTC_PI  is already defined. */

/* RTC.PITDBGCTRL  bit masks and bit positions */
/* RTC_DBGRUN  is already defined. */


/* Real-Time Counter */
typedef struct RTC_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t STATUS;  /* Status */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t TEMP;  /* Temporary */
    register8_t DBGCTRL;  /* Debug control */
    register8_t CALIB;  /* Calibration */
    register8_t CLKSEL;  /* Clock Select */
    _WORDREGISTER(CNT);  /* Counter */
    _WORDREGISTER(PER);  /* Period */
    _WORDREGISTER(CMP);  /* Compare */
    register8_t reserved_1[2];
    register8_t PITCTRLA;  /* PIT Control A */
    register8_t PITSTATUS;  /* PIT Status */
    register8_t PITINTCTRL;  /* PIT Interrupt Control */
    register8_t PITINTFLAGS;  /* PIT Interrupt Flags */
    register8_t reserved_2[1];
    register8_t PITDBGCTRL;  /* PIT Debug control */
    register8_t reserved_3[10];
} RTC_t;

/* Clock Select bit group values */
typedef enum RTC_CLKSEL_VALUES_enum
{
    RTC_CLKSEL_INT32K_gv           = 0x00,  /* Internal 32kHz OSC */
    RTC_CLKSEL_INT1K_gv            = 0x01,  /* Internal 1kHz OSC */
    RTC_CLKSEL_TOSC32K_gv          = 0x02,  /* 32KHz Crystal OSC */
    RTC_CLKSEL_EXTCLK_gv           = 0x03   /* External Clock */
} RTC_CLKSEL_values_t;

/* Clock Select bit group configurations*/
typedef enum RTC_CLKSEL_enum
{
    RTC_CLKSEL_INT32K_gc           = (RTC_CLKSEL_INT32K_gv << RTC_CLKSEL_gp),  /* Internal 32kHz OSC */
    RTC_CLKSEL_INT1K_gc            = (RTC_CLKSEL_INT1K_gv << RTC_CLKSEL_gp),  /* Internal 1kHz OSC */
    RTC_CLKSEL_TOSC32K_gc          = (RTC_CLKSEL_TOSC32K_gv << RTC_CLKSEL_gp),  /* 32KHz Crystal OSC */
    RTC_CLKSEL_EXTCLK_gc           = (RTC_CLKSEL_EXTCLK_gv << RTC_CLKSEL_gp)   /* External Clock */
} RTC_CLKSEL_t;

/* Period select bit group values */
typedef enum RTC_PERIOD_VALUES_enum
{
    RTC_PERIOD_OFF_gv              = 0x00,  /* Off */
    RTC_PERIOD_CYC4_gv             = 0x01,  /* RTC Clock Cycles 4 */
    RTC_PERIOD_CYC8_gv             = 0x02,  /* RTC Clock Cycles 8 */
    RTC_PERIOD_CYC16_gv            = 0x03,  /* RTC Clock Cycles 16 */
    RTC_PERIOD_CYC32_gv            = 0x04,  /* RTC Clock Cycles 32 */
    RTC_PERIOD_CYC64_gv            = 0x05,  /* RTC Clock Cycles 64 */
    RTC_PERIOD_CYC128_gv           = 0x06,  /* RTC Clock Cycles 128 */
    RTC_PERIOD_CYC256_gv           = 0x07,  /* RTC Clock Cycles 256 */
    RTC_PERIOD_CYC512_gv           = 0x08,  /* RTC Clock Cycles 512 */
    RTC_PERIOD_CYC1024_gv          = 0x09,  /* RTC Clock Cycles 1024 */
    RTC_PERIOD_CYC2048_gv          = 0x0A,  /* RTC Clock Cycles 2048 */
    RTC_PERIOD_CYC4096_gv          = 0x0B,  /* RTC Clock Cycles 4096 */
    RTC_PERIOD_CYC8192_gv          = 0x0C,  /* RTC Clock Cycles 8192 */
    RTC_PERIOD_CYC16384_gv         = 0x0D,  /* RTC Clock Cycles 16384 */
    RTC_PERIOD_CYC32768_gv         = 0x0E   /* RTC Clock Cycles 32768 */
} RTC_PERIOD_values_t;

/* Period select bit group configurations*/
typedef enum RTC_PERIOD_enum
{
    RTC_PERIOD_OFF_gc              = (RTC_PERIOD_OFF_gv << RTC_PERIOD_gp),  /* Off */
    RTC_PERIOD_CYC4_gc             = (RTC_PERIOD_CYC4_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 4 */
    RTC_PERIOD_CYC8_gc             = (RTC_PERIOD_CYC8_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 8 */
    RTC_PERIOD_CYC16_gc            = (RTC_PERIOD_CYC16_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 16 */
    RTC_PERIOD_CYC32_gc            = (RTC_PERIOD_CYC32_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 32 */
    RTC_PERIOD_CYC64_gc            = (RTC_PERIOD_CYC64_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 64 */
    RTC_PERIOD_CYC128_gc           = (RTC_PERIOD_CYC128_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 128 */
    RTC_PERIOD_CYC256_gc           = (RTC_PERIOD_CYC256_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 256 */
    RTC_PERIOD_CYC512_gc           = (RTC_PERIOD_CYC512_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 512 */
    RTC_PERIOD_CYC1024_gc          = (RTC_PERIOD_CYC1024_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 1024 */
    RTC_PERIOD_CYC2048_gc          = (RTC_PERIOD_CYC2048_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 2048 */
    RTC_PERIOD_CYC4096_gc          = (RTC_PERIOD_CYC4096_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 4096 */
    RTC_PERIOD_CYC8192_gc          = (RTC_PERIOD_CYC8192_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 8192 */
    RTC_PERIOD_CYC16384_gc         = (RTC_PERIOD_CYC16384_gv << RTC_PERIOD_gp),  /* RTC Clock Cycles 16384 */
    RTC_PERIOD_CYC32768_gc         = (RTC_PERIOD_CYC32768_gv << RTC_PERIOD_gp)   /* RTC Clock Cycles 32768 */
} RTC_PERIOD_t;

/* Prescaling Factor select bit group values */
typedef enum RTC_PRESCALER_VALUES_enum
{
    RTC_PRESCALER_DIV1_gv          = 0x00,  /* RTC Clock / 1 */
    RTC_PRESCALER_DIV2_gv          = 0x01,  /* RTC Clock / 2 */
    RTC_PRESCALER_DIV4_gv          = 0x02,  /* RTC Clock / 4 */
    RTC_PRESCALER_DIV8_gv          = 0x03,  /* RTC Clock / 8 */
    RTC_PRESCALER_DIV16_gv         = 0x04,  /* RTC Clock / 16 */
    RTC_PRESCALER_DIV32_gv         = 0x05,  /* RTC Clock / 32 */
    RTC_PRESCALER_DIV64_gv         = 0x06,  /* RTC Clock / 64 */
    RTC_PRESCALER_DIV128_gv        = 0x07,  /* RTC Clock / 128 */
    RTC_PRESCALER_DIV256_gv        = 0x08,  /* RTC Clock / 256 */
    RTC_PRESCALER_DIV512_gv        = 0x09,  /* RTC Clock / 512 */
    RTC_PRESCALER_DIV1024_gv       = 0x0A,  /* RTC Clock / 1024 */
    RTC_PRESCALER_DIV2048_gv       = 0x0B,  /* RTC Clock / 2048 */
    RTC_PRESCALER_DIV4096_gv       = 0x0C,  /* RTC Clock / 4096 */
    RTC_PRESCALER_DIV8192_gv       = 0x0D,  /* RTC Clock / 8192 */
    RTC_PRESCALER_DIV16384_gv      = 0x0E,  /* RTC Clock / 16384 */
    RTC_PRESCALER_DIV32768_gv      = 0x0F   /* RTC Clock / 32768 */
} RTC_PRESCALER_values_t;

/* Prescaling Factor select bit group configurations*/
typedef enum RTC_PRESCALER_enum
{
    RTC_PRESCALER_DIV1_gc          = (RTC_PRESCALER_DIV1_gv << RTC_PRESCALER_gp),  /* RTC Clock / 1 */
    RTC_PRESCALER_DIV2_gc          = (RTC_PRESCALER_DIV2_gv << RTC_PRESCALER_gp),  /* RTC Clock / 2 */
    RTC_PRESCALER_DIV4_gc          = (RTC_PRESCALER_DIV4_gv << RTC_PRESCALER_gp),  /* RTC Clock / 4 */
    RTC_PRESCALER_DIV8_gc          = (RTC_PRESCALER_DIV8_gv << RTC_PRESCALER_gp),  /* RTC Clock / 8 */
    RTC_PRESCALER_DIV16_gc         = (RTC_PRESCALER_DIV16_gv << RTC_PRESCALER_gp),  /* RTC Clock / 16 */
    RTC_PRESCALER_DIV32_gc         = (RTC_PRESCALER_DIV32_gv << RTC_PRESCALER_gp),  /* RTC Clock / 32 */
    RTC_PRESCALER_DIV64_gc         = (RTC_PRESCALER_DIV64_gv << RTC_PRESCALER_gp),  /* RTC Clock / 64 */
    RTC_PRESCALER_DIV128_gc        = (RTC_PRESCALER_DIV128_gv << RTC_PRESCALER_gp),  /* RTC Clock / 128 */
    RTC_PRESCALER_DIV256_gc        = (RTC_PRESCALER_DIV256_gv << RTC_PRESCALER_gp),  /* RTC Clock / 256 */
    RTC_PRESCALER_DIV512_gc        = (RTC_PRESCALER_DIV512_gv << RTC_PRESCALER_gp),  /* RTC Clock / 512 */
    RTC_PRESCALER_DIV1024_gc       = (RTC_PRESCALER_DIV1024_gv << RTC_PRESCALER_gp),  /* RTC Clock / 1024 */
    RTC_PRESCALER_DIV2048_gc       = (RTC_PRESCALER_DIV2048_gv << RTC_PRESCALER_gp),  /* RTC Clock / 2048 */
    RTC_PRESCALER_DIV4096_gc       = (RTC_PRESCALER_DIV4096_gv << RTC_PRESCALER_gp),  /* RTC Clock / 4096 */
    RTC_PRESCALER_DIV8192_gc       = (RTC_PRESCALER_DIV8192_gv << RTC_PRESCALER_gp),  /* RTC Clock / 8192 */
    RTC_PRESCALER_DIV16384_gc      = (RTC_PRESCALER_DIV16384_gv << RTC_PRESCALER_gp),  /* RTC Clock / 16384 */
    RTC_PRESCALER_DIV32768_gc      = (RTC_PRESCALER_DIV32768_gv << RTC_PRESCALER_gp)   /* RTC Clock / 32768 */
} RTC_PRESCALER_t;

/* End of extract */

#endif //_XT_IO_RTC_H_
