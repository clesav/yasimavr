/*
 * io_tcb.h
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

#ifndef _XT_IO_TCB_H_
#define _XT_IO_TCB_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the TCB peripheral for the following device models:
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


/* TCB - 16-bit Timer Type B */
/* TCB.CTRLA  bit masks and bit positions */
#define TCB_ENABLE_bm  0x01  /* Enable bit mask. */
#define TCB_ENABLE_bp  0  /* Enable bit position. */
#define TCB_CLKSEL_gm  0x0E  /* Clock Select group mask. */
#define TCB_CLKSEL_gp  1  /* Clock Select group position. */
#define TCB_CLKSEL_0_bm  (1<<1)  /* Clock Select bit 0 mask. */
#define TCB_CLKSEL_0_bp  1  /* Clock Select bit 0 position. */
#define TCB_CLKSEL_1_bm  (1<<2)  /* Clock Select bit 1 mask. */
#define TCB_CLKSEL_1_bp  2  /* Clock Select bit 1 position. */
#define TCB_CLKSEL_2_bm  (1<<3)  /* Clock Select bit 2 mask. */
#define TCB_CLKSEL_2_bp  3  /* Clock Select bit 2 position. */
#define TCB_SYNCUPD_bm  0x10  /* Synchronize Update bit mask. */
#define TCB_SYNCUPD_bp  4  /* Synchronize Update bit position. */
#define TCB_CASCADE_bm  0x20  /* Cascade two timers bit mask. */
#define TCB_CASCADE_bp  5  /* Cascade two timers bit position. */
#define TCB_RUNSTDBY_bm  0x40  /* Run Standby bit mask. */
#define TCB_RUNSTDBY_bp  6  /* Run Standby bit position. */

/* TCB.CTRLB  bit masks and bit positions */
#define TCB_CNTMODE_gm  0x07  /* Timer Mode group mask. */
#define TCB_CNTMODE_gp  0  /* Timer Mode group position. */
#define TCB_CNTMODE_0_bm  (1<<0)  /* Timer Mode bit 0 mask. */
#define TCB_CNTMODE_0_bp  0  /* Timer Mode bit 0 position. */
#define TCB_CNTMODE_1_bm  (1<<1)  /* Timer Mode bit 1 mask. */
#define TCB_CNTMODE_1_bp  1  /* Timer Mode bit 1 position. */
#define TCB_CNTMODE_2_bm  (1<<2)  /* Timer Mode bit 2 mask. */
#define TCB_CNTMODE_2_bp  2  /* Timer Mode bit 2 position. */
#define TCB_CCMPEN_bm  0x10  /* Pin Output Enable bit mask. */
#define TCB_CCMPEN_bp  4  /* Pin Output Enable bit position. */
#define TCB_CCMPINIT_bm  0x20  /* Pin Initial State bit mask. */
#define TCB_CCMPINIT_bp  5  /* Pin Initial State bit position. */
#define TCB_ASYNC_bm  0x40  /* Asynchronous Enable bit mask. */
#define TCB_ASYNC_bp  6  /* Asynchronous Enable bit position. */

/* TCB.EVCTRL  bit masks and bit positions */
#define TCB_CAPTEI_bm  0x01  /* Event Input Enable bit mask. */
#define TCB_CAPTEI_bp  0  /* Event Input Enable bit position. */
#define TCB_EDGE_bm  0x10  /* Event Edge bit mask. */
#define TCB_EDGE_bp  4  /* Event Edge bit position. */
#define TCB_FILTER_bm  0x40  /* Input Capture Noise Cancellation Filter bit mask. */
#define TCB_FILTER_bp  6  /* Input Capture Noise Cancellation Filter bit position. */

/* TCB.INTCTRL  bit masks and bit positions */
#define TCB_CAPT_bm  0x01  /* Capture or Timeout bit mask. */
#define TCB_CAPT_bp  0  /* Capture or Timeout bit position. */
#define TCB_OVF_bm  0x02  /* Overflow bit mask. */
#define TCB_OVF_bp  1  /* Overflow bit position. */

/* TCB.INTFLAGS  bit masks and bit positions */
/* TCB_CAPT  is already defined. */
/* TCB_OVF  is already defined. */

/* TCB.STATUS  bit masks and bit positions */
#define TCB_RUN_bm  0x01  /* Run bit mask. */
#define TCB_RUN_bp  0  /* Run bit position. */

/* TCB.DBGCTRL  bit masks and bit positions */
#define TCB_DBGRUN_bm  0x01  /* Debug Run bit mask. */
#define TCB_DBGRUN_bp  0  /* Debug Run bit position. */


/* 16-bit Timer Type B */
typedef struct TCB_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control Register B */
    register8_t reserved_1[2];
    register8_t EVCTRL;  /* Event Control */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t STATUS;  /* Status */
    register8_t DBGCTRL;  /* Debug Control */
    register8_t TEMP;  /* Temporary Value */
    _WORDREGISTER(CNT);  /* Count */
    _WORDREGISTER(CCMP);  /* Compare or Capture */
    register8_t reserved_2[2];
} TCB_t;

/* Clock Select bit group values */
typedef enum TCB_CLKSEL_VALUES_enum
{
    TCB_CLKSEL_DIV1_gv             = 0x00,  /* CLK_PER */
    TCB_CLKSEL_DIV2_gv             = 0x01,  /* CLK_PER/2 */
    TCB_CLKSEL_TCA0_gv             = 0x02,  /* Use CLK_TCA from TCA0 */
    TCB_CLKSEL_EVENT_gv            = 0x07   /* Count on event edge */
} TCB_CLKSEL_values_t;

/* Clock Select bit group configurations*/
typedef enum TCB_CLKSEL_enum
{
    TCB_CLKSEL_DIV1_gc             = (TCB_CLKSEL_DIV1_gv << TCB_CLKSEL_gp),  /* CLK_PER */
    TCB_CLKSEL_DIV2_gc             = (TCB_CLKSEL_DIV2_gv << TCB_CLKSEL_gp),  /* CLK_PER/2 */
    TCB_CLKSEL_TCA0_gc             = (TCB_CLKSEL_TCA0_gv << TCB_CLKSEL_gp),  /* Use CLK_TCA from TCA0 */
    TCB_CLKSEL_EVENT_gc            = (TCB_CLKSEL_EVENT_gv << TCB_CLKSEL_gp)   /* Count on event edge */
} TCB_CLKSEL_t;

/* Timer Mode select bit group values */
typedef enum TCB_CNTMODE_VALUES_enum
{
    TCB_CNTMODE_INT_gv             = 0x00,  /* Periodic Interrupt */
    TCB_CNTMODE_TIMEOUT_gv         = 0x01,  /* Periodic Timeout */
    TCB_CNTMODE_CAPT_gv            = 0x02,  /* Input Capture Event */
    TCB_CNTMODE_FRQ_gv             = 0x03,  /* Input Capture Frequency measurement */
    TCB_CNTMODE_PW_gv              = 0x04,  /* Input Capture Pulse-Width measurement */
    TCB_CNTMODE_FRQPW_gv           = 0x05,  /* Input Capture Frequency and Pulse-Width measurement */
    TCB_CNTMODE_SINGLE_gv          = 0x06,  /* Single Shot */
    TCB_CNTMODE_PWM8_gv            = 0x07   /* 8-bit PWM */
} TCB_CNTMODE_values_t;

/* Timer Mode select bit group configurations*/
typedef enum TCB_CNTMODE_enum
{
    TCB_CNTMODE_INT_gc             = (TCB_CNTMODE_INT_gv << TCB_CNTMODE_gp),  /* Periodic Interrupt */
    TCB_CNTMODE_TIMEOUT_gc         = (TCB_CNTMODE_TIMEOUT_gv << TCB_CNTMODE_gp),  /* Periodic Timeout */
    TCB_CNTMODE_CAPT_gc            = (TCB_CNTMODE_CAPT_gv << TCB_CNTMODE_gp),  /* Input Capture Event */
    TCB_CNTMODE_FRQ_gc             = (TCB_CNTMODE_FRQ_gv << TCB_CNTMODE_gp),  /* Input Capture Frequency measurement */
    TCB_CNTMODE_PW_gc              = (TCB_CNTMODE_PW_gv << TCB_CNTMODE_gp),  /* Input Capture Pulse-Width measurement */
    TCB_CNTMODE_FRQPW_gc           = (TCB_CNTMODE_FRQPW_gv << TCB_CNTMODE_gp),  /* Input Capture Frequency and Pulse-Width measurement */
    TCB_CNTMODE_SINGLE_gc          = (TCB_CNTMODE_SINGLE_gv << TCB_CNTMODE_gp),  /* Single Shot */
    TCB_CNTMODE_PWM8_gc            = (TCB_CNTMODE_PWM8_gv << TCB_CNTMODE_gp)   /* 8-bit PWM */
} TCB_CNTMODE_t;

/* End of extract */

#endif //_XT_IO_TCB_H_
