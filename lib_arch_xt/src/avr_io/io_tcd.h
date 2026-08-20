/*
 * io_tcd.h
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

#ifndef _XT_IO_TCD_H_
#define _XT_IO_TCD_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the TCD peripheral for the following device models:
 *  - attiny 1-series
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


/* TCD.CTRLA  bit masks and bit positions */
#define TCD_ENABLE_bm  0x01  /* Enable bit mask. */
#define TCD_ENABLE_bp  0  /* Enable bit position. */
#define TCD_SYNCPRES_gm  0x06  /* Synchronization prescaler group mask. */
#define TCD_SYNCPRES_gp  1  /* Synchronization prescaler group position. */
#define TCD_SYNCPRES_0_bm  (1<<1)  /* Synchronization prescaler bit 0 mask. */
#define TCD_SYNCPRES_0_bp  1  /* Synchronization prescaler bit 0 position. */
#define TCD_SYNCPRES_1_bm  (1<<2)  /* Synchronization prescaler bit 1 mask. */
#define TCD_SYNCPRES_1_bp  2  /* Synchronization prescaler bit 1 position. */
#define TCD_CNTPRES_gm  0x18  /* counter prescaler group mask. */
#define TCD_CNTPRES_gp  3  /* counter prescaler group position. */
#define TCD_CNTPRES_0_bm  (1<<3)  /* counter prescaler bit 0 mask. */
#define TCD_CNTPRES_0_bp  3  /* counter prescaler bit 0 position. */
#define TCD_CNTPRES_1_bm  (1<<4)  /* counter prescaler bit 1 mask. */
#define TCD_CNTPRES_1_bp  4  /* counter prescaler bit 1 position. */
#define TCD_CLKSEL_gm  0x60  /* clock select group mask. */
#define TCD_CLKSEL_gp  5  /* clock select group position. */
#define TCD_CLKSEL_0_bm  (1<<5)  /* clock select bit 0 mask. */
#define TCD_CLKSEL_0_bp  5  /* clock select bit 0 position. */
#define TCD_CLKSEL_1_bm  (1<<6)  /* clock select bit 1 mask. */
#define TCD_CLKSEL_1_bp  6  /* clock select bit 1 position. */

/* TCD.CTRLB  bit masks and bit positions */
#define TCD_WGMODE_gm  0x03  /* Waveform generation mode group mask. */
#define TCD_WGMODE_gp  0  /* Waveform generation mode group position. */
#define TCD_WGMODE_0_bm  (1<<0)  /* Waveform generation mode bit 0 mask. */
#define TCD_WGMODE_0_bp  0  /* Waveform generation mode bit 0 position. */
#define TCD_WGMODE_1_bm  (1<<1)  /* Waveform generation mode bit 1 mask. */
#define TCD_WGMODE_1_bp  1  /* Waveform generation mode bit 1 position. */

/* TCD.CTRLC  bit masks and bit positions */
#define TCD_CMPOVR_bm  0x01  /* Compare output value override bit mask. */
#define TCD_CMPOVR_bp  0  /* Compare output value override bit position. */
#define TCD_AUPDATE_bm  0x02  /* Auto update bit mask. */
#define TCD_AUPDATE_bp  1  /* Auto update bit position. */
#define TCD_FIFTY_bm  0x08  /* Fifty percent waveform bit mask. */
#define TCD_FIFTY_bp  3  /* Fifty percent waveform bit position. */
#define TCD_CMPCSEL_bm  0x40  /* Compare C output select bit mask. */
#define TCD_CMPCSEL_bp  6  /* Compare C output select bit position. */
#define TCD_CMPDSEL_bm  0x80  /* Compare D output select bit mask. */
#define TCD_CMPDSEL_bp  7  /* Compare D output select bit position. */

/* TCD.CTRLD  bit masks and bit positions */
#define TCD_CMPAVAL_gm  0x0F  /* Compare A value group mask. */
#define TCD_CMPAVAL_gp  0  /* Compare A value group position. */
#define TCD_CMPAVAL_0_bm  (1<<0)  /* Compare A value bit 0 mask. */
#define TCD_CMPAVAL_0_bp  0  /* Compare A value bit 0 position. */
#define TCD_CMPAVAL_1_bm  (1<<1)  /* Compare A value bit 1 mask. */
#define TCD_CMPAVAL_1_bp  1  /* Compare A value bit 1 position. */
#define TCD_CMPAVAL_2_bm  (1<<2)  /* Compare A value bit 2 mask. */
#define TCD_CMPAVAL_2_bp  2  /* Compare A value bit 2 position. */
#define TCD_CMPAVAL_3_bm  (1<<3)  /* Compare A value bit 3 mask. */
#define TCD_CMPAVAL_3_bp  3  /* Compare A value bit 3 position. */
#define TCD_CMPBVAL_gm  0xF0  /* Compare B value group mask. */
#define TCD_CMPBVAL_gp  4  /* Compare B value group position. */
#define TCD_CMPBVAL_0_bm  (1<<4)  /* Compare B value bit 0 mask. */
#define TCD_CMPBVAL_0_bp  4  /* Compare B value bit 0 position. */
#define TCD_CMPBVAL_1_bm  (1<<5)  /* Compare B value bit 1 mask. */
#define TCD_CMPBVAL_1_bp  5  /* Compare B value bit 1 position. */
#define TCD_CMPBVAL_2_bm  (1<<6)  /* Compare B value bit 2 mask. */
#define TCD_CMPBVAL_2_bp  6  /* Compare B value bit 2 position. */
#define TCD_CMPBVAL_3_bm  (1<<7)  /* Compare B value bit 3 mask. */
#define TCD_CMPBVAL_3_bp  7  /* Compare B value bit 3 position. */

/* TCD.CTRLE  bit masks and bit positions */
#define TCD_SYNCEOC_bm  0x01  /* Synchronize end of cycle strobe bit mask. */
#define TCD_SYNCEOC_bp  0  /* Synchronize end of cycle strobe bit position. */
#define TCD_SYNC_bm  0x02  /* Synchronize strobe bit mask. */
#define TCD_SYNC_bp  1  /* Synchronize strobe bit position. */
#define TCD_RESTART_bm  0x04  /* Restart strobe bit mask. */
#define TCD_RESTART_bp  2  /* Restart strobe bit position. */
#define TCD_SCAPTUREA_bm  0x08  /* Software Capture A Strobe bit mask. */
#define TCD_SCAPTUREA_bp  3  /* Software Capture A Strobe bit position. */
#define TCD_SCAPTUREB_bm  0x10  /* Software Capture B Strobe bit mask. */
#define TCD_SCAPTUREB_bp  4  /* Software Capture B Strobe bit position. */
#define TCD_DISEOC_bm  0x80  /* Disable at end of cycle bit mask. */
#define TCD_DISEOC_bp  7  /* Disable at end of cycle bit position. */

/* TCD.EVCTRLA  bit masks and bit positions */
#define TCD_TRIGEI_bm  0x01  /* Trigger event enable bit mask. */
#define TCD_TRIGEI_bp  0  /* Trigger event enable bit position. */
#define TCD_ACTION_bm  0x04  /* Event action bit mask. */
#define TCD_ACTION_bp  2  /* Event action bit position. */
#define TCD_EDGE_bm  0x10  /* Edge select bit mask. */
#define TCD_EDGE_bp  4  /* Edge select bit position. */
#define TCD_CFG_gm  0xC0  /* Event config group mask. */
#define TCD_CFG_gp  6  /* Event config group position. */
#define TCD_CFG_0_bm  (1<<6)  /* Event config bit 0 mask. */
#define TCD_CFG_0_bp  6  /* Event config bit 0 position. */
#define TCD_CFG_1_bm  (1<<7)  /* Event config bit 1 mask. */
#define TCD_CFG_1_bp  7  /* Event config bit 1 position. */

/* TCD.EVCTRLB  bit masks and bit positions */
/* TCD_TRIGEI  is already defined. */
/* TCD_ACTION  is already defined. */
/* TCD_EDGE  is already defined. */
/* TCD_CFG  is already defined. */

/* TCD.INTCTRL  bit masks and bit positions */
#define TCD_OVF_bm  0x01  /* Overflow interrupt enable bit mask. */
#define TCD_OVF_bp  0  /* Overflow interrupt enable bit position. */
#define TCD_TRIGA_bm  0x04  /* Trigger A interrupt enable bit mask. */
#define TCD_TRIGA_bp  2  /* Trigger A interrupt enable bit position. */
#define TCD_TRIGB_bm  0x08  /* Trigger B interrupt enable bit mask. */
#define TCD_TRIGB_bp  3  /* Trigger B interrupt enable bit position. */

/* TCD.INTFLAGS  bit masks and bit positions */
/* TCD_OVF  is already defined. */
/* TCD_TRIGA  is already defined. */
/* TCD_TRIGB  is already defined. */

/* TCD.STATUS  bit masks and bit positions */
#define TCD_ENRDY_bm  0x01  /* Enable ready bit mask. */
#define TCD_ENRDY_bp  0  /* Enable ready bit position. */
#define TCD_CMDRDY_bm  0x02  /* Command ready bit mask. */
#define TCD_CMDRDY_bp  1  /* Command ready bit position. */
#define TCD_PWMACTA_bm  0x40  /* PWM activity on A bit mask. */
#define TCD_PWMACTA_bp  6  /* PWM activity on A bit position. */
#define TCD_PWMACTB_bm  0x80  /* PWM activity on B bit mask. */
#define TCD_PWMACTB_bp  7  /* PWM activity on B bit position. */

/* TCD.INPUTCTRLA  bit masks and bit positions */
#define TCD_INPUTMODE_gm  0x0F  /* Input mode group mask. */
#define TCD_INPUTMODE_gp  0  /* Input mode group position. */
#define TCD_INPUTMODE_0_bm  (1<<0)  /* Input mode bit 0 mask. */
#define TCD_INPUTMODE_0_bp  0  /* Input mode bit 0 position. */
#define TCD_INPUTMODE_1_bm  (1<<1)  /* Input mode bit 1 mask. */
#define TCD_INPUTMODE_1_bp  1  /* Input mode bit 1 position. */
#define TCD_INPUTMODE_2_bm  (1<<2)  /* Input mode bit 2 mask. */
#define TCD_INPUTMODE_2_bp  2  /* Input mode bit 2 position. */
#define TCD_INPUTMODE_3_bm  (1<<3)  /* Input mode bit 3 mask. */
#define TCD_INPUTMODE_3_bp  3  /* Input mode bit 3 position. */

/* TCD.INPUTCTRLB  bit masks and bit positions */
/* TCD_INPUTMODE  is already defined. */

/* TCD.FAULTCTRL  bit masks and bit positions */
#define TCD_CMPA_bm  0x01  /* Compare A value bit mask. */
#define TCD_CMPA_bp  0  /* Compare A value bit position. */
#define TCD_CMPB_bm  0x02  /* Compare B value bit mask. */
#define TCD_CMPB_bp  1  /* Compare B value bit position. */
#define TCD_CMPC_bm  0x04  /* Compare C value bit mask. */
#define TCD_CMPC_bp  2  /* Compare C value bit position. */
#define TCD_CMPD_bm  0x08  /* Compare D vaule bit mask. */
#define TCD_CMPD_bp  3  /* Compare D vaule bit position. */
#define TCD_CMPAEN_bm  0x10  /* Compare A enable bit mask. */
#define TCD_CMPAEN_bp  4  /* Compare A enable bit position. */
#define TCD_CMPBEN_bm  0x20  /* Compare B enable bit mask. */
#define TCD_CMPBEN_bp  5  /* Compare B enable bit position. */
#define TCD_CMPCEN_bm  0x40  /* Compare C enable bit mask. */
#define TCD_CMPCEN_bp  6  /* Compare C enable bit position. */
#define TCD_CMPDEN_bm  0x80  /* Compare D enable bit mask. */
#define TCD_CMPDEN_bp  7  /* Compare D enable bit position. */

/* TCD.DLYCTRL  bit masks and bit positions */
#define TCD_DLYSEL_gm  0x03  /* Delay select group mask. */
#define TCD_DLYSEL_gp  0  /* Delay select group position. */
#define TCD_DLYSEL_0_bm  (1<<0)  /* Delay select bit 0 mask. */
#define TCD_DLYSEL_0_bp  0  /* Delay select bit 0 position. */
#define TCD_DLYSEL_1_bm  (1<<1)  /* Delay select bit 1 mask. */
#define TCD_DLYSEL_1_bp  1  /* Delay select bit 1 position. */
#define TCD_DLYTRIG_gm  0x0C  /* Delay trigger group mask. */
#define TCD_DLYTRIG_gp  2  /* Delay trigger group position. */
#define TCD_DLYTRIG_0_bm  (1<<2)  /* Delay trigger bit 0 mask. */
#define TCD_DLYTRIG_0_bp  2  /* Delay trigger bit 0 position. */
#define TCD_DLYTRIG_1_bm  (1<<3)  /* Delay trigger bit 1 mask. */
#define TCD_DLYTRIG_1_bp  3  /* Delay trigger bit 1 position. */
#define TCD_DLYPRESC_gm  0x30  /* Delay prescaler group mask. */
#define TCD_DLYPRESC_gp  4  /* Delay prescaler group position. */
#define TCD_DLYPRESC_0_bm  (1<<4)  /* Delay prescaler bit 0 mask. */
#define TCD_DLYPRESC_0_bp  4  /* Delay prescaler bit 0 position. */
#define TCD_DLYPRESC_1_bm  (1<<5)  /* Delay prescaler bit 1 mask. */
#define TCD_DLYPRESC_1_bp  5  /* Delay prescaler bit 1 position. */

/* TCD.DLYVAL  bit masks and bit positions */
#define TCD_DLYVAL_gm  0xFF  /* Delay value group mask. */
#define TCD_DLYVAL_gp  0  /* Delay value group position. */
#define TCD_DLYVAL_0_bm  (1<<0)  /* Delay value bit 0 mask. */
#define TCD_DLYVAL_0_bp  0  /* Delay value bit 0 position. */
#define TCD_DLYVAL_1_bm  (1<<1)  /* Delay value bit 1 mask. */
#define TCD_DLYVAL_1_bp  1  /* Delay value bit 1 position. */
#define TCD_DLYVAL_2_bm  (1<<2)  /* Delay value bit 2 mask. */
#define TCD_DLYVAL_2_bp  2  /* Delay value bit 2 position. */
#define TCD_DLYVAL_3_bm  (1<<3)  /* Delay value bit 3 mask. */
#define TCD_DLYVAL_3_bp  3  /* Delay value bit 3 position. */
#define TCD_DLYVAL_4_bm  (1<<4)  /* Delay value bit 4 mask. */
#define TCD_DLYVAL_4_bp  4  /* Delay value bit 4 position. */
#define TCD_DLYVAL_5_bm  (1<<5)  /* Delay value bit 5 mask. */
#define TCD_DLYVAL_5_bp  5  /* Delay value bit 5 position. */
#define TCD_DLYVAL_6_bm  (1<<6)  /* Delay value bit 6 mask. */
#define TCD_DLYVAL_6_bp  6  /* Delay value bit 6 position. */
#define TCD_DLYVAL_7_bm  (1<<7)  /* Delay value bit 7 mask. */
#define TCD_DLYVAL_7_bp  7  /* Delay value bit 7 position. */

/* TCD.DITCTRL  bit masks and bit positions */
#define TCD_DITHERSEL_gm  0x03  /* Dither select group mask. */
#define TCD_DITHERSEL_gp  0  /* Dither select group position. */
#define TCD_DITHERSEL_0_bm  (1<<0)  /* Dither select bit 0 mask. */
#define TCD_DITHERSEL_0_bp  0  /* Dither select bit 0 position. */
#define TCD_DITHERSEL_1_bm  (1<<1)  /* Dither select bit 1 mask. */
#define TCD_DITHERSEL_1_bp  1  /* Dither select bit 1 position. */

/* TCD.DITVAL  bit masks and bit positions */
#define TCD_DITHER_gm  0x0F  /* Dither value group mask. */
#define TCD_DITHER_gp  0  /* Dither value group position. */
#define TCD_DITHER_0_bm  (1<<0)  /* Dither value bit 0 mask. */
#define TCD_DITHER_0_bp  0  /* Dither value bit 0 position. */
#define TCD_DITHER_1_bm  (1<<1)  /* Dither value bit 1 mask. */
#define TCD_DITHER_1_bp  1  /* Dither value bit 1 position. */
#define TCD_DITHER_2_bm  (1<<2)  /* Dither value bit 2 mask. */
#define TCD_DITHER_2_bp  2  /* Dither value bit 2 position. */
#define TCD_DITHER_3_bm  (1<<3)  /* Dither value bit 3 mask. */
#define TCD_DITHER_3_bp  3  /* Dither value bit 3 position. */

/* TCD.DBGCTRL  bit masks and bit positions */
#define TCD_DBGRUN_bm  0x01  /* Debug run bit mask. */
#define TCD_DBGRUN_bp  0  /* Debug run bit position. */
#define TCD_FAULTDET_bm  0x04  /* Fault detection bit mask. */
#define TCD_FAULTDET_bp  2  /* Fault detection bit position. */

/* TCD.CMPASET  bit masks and bit positions */
#define TCD_CMPASET_gm  0xFFF  /* Compare A Set group mask. */
#define TCD_CMPASET_gp  0  /* Compare A Set group position. */
#define TCD_CMPASET_0_bm  (1<<0)  /* Compare A Set bit 0 mask. */
#define TCD_CMPASET_0_bp  0  /* Compare A Set bit 0 position. */
#define TCD_CMPASET_1_bm  (1<<1)  /* Compare A Set bit 1 mask. */
#define TCD_CMPASET_1_bp  1  /* Compare A Set bit 1 position. */
#define TCD_CMPASET_2_bm  (1<<2)  /* Compare A Set bit 2 mask. */
#define TCD_CMPASET_2_bp  2  /* Compare A Set bit 2 position. */
#define TCD_CMPASET_3_bm  (1<<3)  /* Compare A Set bit 3 mask. */
#define TCD_CMPASET_3_bp  3  /* Compare A Set bit 3 position. */
#define TCD_CMPASET_4_bm  (1<<4)  /* Compare A Set bit 4 mask. */
#define TCD_CMPASET_4_bp  4  /* Compare A Set bit 4 position. */
#define TCD_CMPASET_5_bm  (1<<5)  /* Compare A Set bit 5 mask. */
#define TCD_CMPASET_5_bp  5  /* Compare A Set bit 5 position. */
#define TCD_CMPASET_6_bm  (1<<6)  /* Compare A Set bit 6 mask. */
#define TCD_CMPASET_6_bp  6  /* Compare A Set bit 6 position. */
#define TCD_CMPASET_7_bm  (1<<7)  /* Compare A Set bit 7 mask. */
#define TCD_CMPASET_7_bp  7  /* Compare A Set bit 7 position. */
#define TCD_CMPASET_8_bm  (1<<8)  /* Compare A Set bit 8 mask. */
#define TCD_CMPASET_8_bp  8  /* Compare A Set bit 8 position. */
#define TCD_CMPASET_9_bm  (1<<9)  /* Compare A Set bit 9 mask. */
#define TCD_CMPASET_9_bp  9  /* Compare A Set bit 9 position. */
#define TCD_CMPASET_10_bm  (1<<10)  /* Compare A Set bit 10 mask. */
#define TCD_CMPASET_10_bp  10  /* Compare A Set bit 10 position. */
#define TCD_CMPASET_11_bm  (1<<11)  /* Compare A Set bit 11 mask. */
#define TCD_CMPASET_11_bp  11  /* Compare A Set bit 11 position. */

/* TCD.CMPACLR  bit masks and bit positions */
#define TCD_CMPACLR_gm  0xFFF  /* Compare A Set group mask. */
#define TCD_CMPACLR_gp  0  /* Compare A Set group position. */
#define TCD_CMPACLR_0_bm  (1<<0)  /* Compare A Set bit 0 mask. */
#define TCD_CMPACLR_0_bp  0  /* Compare A Set bit 0 position. */
#define TCD_CMPACLR_1_bm  (1<<1)  /* Compare A Set bit 1 mask. */
#define TCD_CMPACLR_1_bp  1  /* Compare A Set bit 1 position. */
#define TCD_CMPACLR_2_bm  (1<<2)  /* Compare A Set bit 2 mask. */
#define TCD_CMPACLR_2_bp  2  /* Compare A Set bit 2 position. */
#define TCD_CMPACLR_3_bm  (1<<3)  /* Compare A Set bit 3 mask. */
#define TCD_CMPACLR_3_bp  3  /* Compare A Set bit 3 position. */
#define TCD_CMPACLR_4_bm  (1<<4)  /* Compare A Set bit 4 mask. */
#define TCD_CMPACLR_4_bp  4  /* Compare A Set bit 4 position. */
#define TCD_CMPACLR_5_bm  (1<<5)  /* Compare A Set bit 5 mask. */
#define TCD_CMPACLR_5_bp  5  /* Compare A Set bit 5 position. */
#define TCD_CMPACLR_6_bm  (1<<6)  /* Compare A Set bit 6 mask. */
#define TCD_CMPACLR_6_bp  6  /* Compare A Set bit 6 position. */
#define TCD_CMPACLR_7_bm  (1<<7)  /* Compare A Set bit 7 mask. */
#define TCD_CMPACLR_7_bp  7  /* Compare A Set bit 7 position. */
#define TCD_CMPACLR_8_bm  (1<<8)  /* Compare A Set bit 8 mask. */
#define TCD_CMPACLR_8_bp  8  /* Compare A Set bit 8 position. */
#define TCD_CMPACLR_9_bm  (1<<9)  /* Compare A Set bit 9 mask. */
#define TCD_CMPACLR_9_bp  9  /* Compare A Set bit 9 position. */
#define TCD_CMPACLR_10_bm  (1<<10)  /* Compare A Set bit 10 mask. */
#define TCD_CMPACLR_10_bp  10  /* Compare A Set bit 10 position. */
#define TCD_CMPACLR_11_bm  (1<<11)  /* Compare A Set bit 11 mask. */
#define TCD_CMPACLR_11_bp  11  /* Compare A Set bit 11 position. */

/* TCD.CMPBSET  bit masks and bit positions */
#define TCD_CMPBSET_gm  0xFFF  /* Compare B Set group mask. */
#define TCD_CMPBSET_gp  0  /* Compare B Set group position. */
#define TCD_CMPBSET_0_bm  (1<<0)  /* Compare B Set bit 0 mask. */
#define TCD_CMPBSET_0_bp  0  /* Compare B Set bit 0 position. */
#define TCD_CMPBSET_1_bm  (1<<1)  /* Compare B Set bit 1 mask. */
#define TCD_CMPBSET_1_bp  1  /* Compare B Set bit 1 position. */
#define TCD_CMPBSET_2_bm  (1<<2)  /* Compare B Set bit 2 mask. */
#define TCD_CMPBSET_2_bp  2  /* Compare B Set bit 2 position. */
#define TCD_CMPBSET_3_bm  (1<<3)  /* Compare B Set bit 3 mask. */
#define TCD_CMPBSET_3_bp  3  /* Compare B Set bit 3 position. */
#define TCD_CMPBSET_4_bm  (1<<4)  /* Compare B Set bit 4 mask. */
#define TCD_CMPBSET_4_bp  4  /* Compare B Set bit 4 position. */
#define TCD_CMPBSET_5_bm  (1<<5)  /* Compare B Set bit 5 mask. */
#define TCD_CMPBSET_5_bp  5  /* Compare B Set bit 5 position. */
#define TCD_CMPBSET_6_bm  (1<<6)  /* Compare B Set bit 6 mask. */
#define TCD_CMPBSET_6_bp  6  /* Compare B Set bit 6 position. */
#define TCD_CMPBSET_7_bm  (1<<7)  /* Compare B Set bit 7 mask. */
#define TCD_CMPBSET_7_bp  7  /* Compare B Set bit 7 position. */
#define TCD_CMPBSET_8_bm  (1<<8)  /* Compare B Set bit 8 mask. */
#define TCD_CMPBSET_8_bp  8  /* Compare B Set bit 8 position. */
#define TCD_CMPBSET_9_bm  (1<<9)  /* Compare B Set bit 9 mask. */
#define TCD_CMPBSET_9_bp  9  /* Compare B Set bit 9 position. */
#define TCD_CMPBSET_10_bm  (1<<10)  /* Compare B Set bit 10 mask. */
#define TCD_CMPBSET_10_bp  10  /* Compare B Set bit 10 position. */
#define TCD_CMPBSET_11_bm  (1<<11)  /* Compare B Set bit 11 mask. */
#define TCD_CMPBSET_11_bp  11  /* Compare B Set bit 11 position. */

/* TCD.CMPBCLR  bit masks and bit positions */
#define TCD_CMPBCLR_gm  0xFFF  /* Compare B Clear group mask. */
#define TCD_CMPBCLR_gp  0  /* Compare B Clear group position. */
#define TCD_CMPBCLR_0_bm  (1<<0)  /* Compare B Clear bit 0 mask. */
#define TCD_CMPBCLR_0_bp  0  /* Compare B Clear bit 0 position. */
#define TCD_CMPBCLR_1_bm  (1<<1)  /* Compare B Clear bit 1 mask. */
#define TCD_CMPBCLR_1_bp  1  /* Compare B Clear bit 1 position. */
#define TCD_CMPBCLR_2_bm  (1<<2)  /* Compare B Clear bit 2 mask. */
#define TCD_CMPBCLR_2_bp  2  /* Compare B Clear bit 2 position. */
#define TCD_CMPBCLR_3_bm  (1<<3)  /* Compare B Clear bit 3 mask. */
#define TCD_CMPBCLR_3_bp  3  /* Compare B Clear bit 3 position. */
#define TCD_CMPBCLR_4_bm  (1<<4)  /* Compare B Clear bit 4 mask. */
#define TCD_CMPBCLR_4_bp  4  /* Compare B Clear bit 4 position. */
#define TCD_CMPBCLR_5_bm  (1<<5)  /* Compare B Clear bit 5 mask. */
#define TCD_CMPBCLR_5_bp  5  /* Compare B Clear bit 5 position. */
#define TCD_CMPBCLR_6_bm  (1<<6)  /* Compare B Clear bit 6 mask. */
#define TCD_CMPBCLR_6_bp  6  /* Compare B Clear bit 6 position. */
#define TCD_CMPBCLR_7_bm  (1<<7)  /* Compare B Clear bit 7 mask. */
#define TCD_CMPBCLR_7_bp  7  /* Compare B Clear bit 7 position. */
#define TCD_CMPBCLR_8_bm  (1<<8)  /* Compare B Clear bit 8 mask. */
#define TCD_CMPBCLR_8_bp  8  /* Compare B Clear bit 8 position. */
#define TCD_CMPBCLR_9_bm  (1<<9)  /* Compare B Clear bit 9 mask. */
#define TCD_CMPBCLR_9_bp  9  /* Compare B Clear bit 9 position. */
#define TCD_CMPBCLR_10_bm  (1<<10)  /* Compare B Clear bit 10 mask. */
#define TCD_CMPBCLR_10_bp  10  /* Compare B Clear bit 10 position. */
#define TCD_CMPBCLR_11_bm  (1<<11)  /* Compare B Clear bit 11 mask. */
#define TCD_CMPBCLR_11_bp  11  /* Compare B Clear bit 11 position. */


/* Timer Counter D */
typedef struct TCD_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t CTRLC;  /* Control C */
    register8_t CTRLD;  /* Control D */
    register8_t CTRLE;  /* Control E */
    register8_t reserved_1[3];
    register8_t EVCTRLA;  /* EVCTRLA */
    register8_t EVCTRLB;  /* EVCTRLB */
    register8_t reserved_2[2];
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t STATUS;  /* Status */
    register8_t reserved_3[1];
    register8_t INPUTCTRLA;  /* Input Control A */
    register8_t INPUTCTRLB;  /* Input Control B */
    register8_t FAULTCTRL;  /* Fault Control */
    register8_t reserved_4[1];
    register8_t DLYCTRL;  /* Delay Control */
    register8_t DLYVAL;  /* Delay value */
    register8_t reserved_5[2];
    register8_t DITCTRL;  /* Dither Control A */
    register8_t DITVAL;  /* Dither value */
    register8_t reserved_6[4];
    register8_t DBGCTRL;  /* Debug Control */
    register8_t reserved_7[3];
    _WORDREGISTER(CAPTUREA);  /* Capture A */
    _WORDREGISTER(CAPTUREB);  /* Capture B */
    register8_t reserved_8[2];
    _WORDREGISTER(CMPASET);  /* Compare A Set */
    _WORDREGISTER(CMPACLR);  /* Compare A Clear */
    _WORDREGISTER(CMPBSET);  /* Compare B Set */
    _WORDREGISTER(CMPBCLR);  /* Compare B Clear */
    register8_t reserved_9[16];
} TCD_t;

/* Event action select bit group values */
typedef enum TCD_ACTION_VALUES_enum
{
    TCD_ACTION_FAULT_gv            = 0x00,  /* Event trigger a fault */
    TCD_ACTION_CAPTURE_gv          = 0x01   /* Event trigger a fault and capture */
} TCD_ACTION_values_t;

/* Event action select bit group configurations*/
typedef enum TCD_ACTION_enum
{
    TCD_ACTION_FAULT_gc            = (TCD_ACTION_FAULT_gv << TCD_ACTION_bp),  /* Event trigger a fault */
    TCD_ACTION_CAPTURE_gc          = (TCD_ACTION_CAPTURE_gv << TCD_ACTION_bp)   /* Event trigger a fault and capture */
} TCD_ACTION_t;

/* Event config select bit group values */
typedef enum TCD_CFG_VALUES_enum
{
    TCD_CFG_NEITHER_gv             = 0x00,  /* Neither Filter nor Asynchronous Event is enabled */
    TCD_CFG_FILTER_gv              = 0x01,  /* Input Capture Noise Cancellation Filter enabled */
    TCD_CFG_ASYNC_gv               = 0x02   /* Asynchronous Event output qualification enabled */
} TCD_CFG_values_t;

/* Event config select bit group configurations*/
typedef enum TCD_CFG_enum
{
    TCD_CFG_NEITHER_gc             = (TCD_CFG_NEITHER_gv << TCD_CFG_gp),  /* Neither Filter nor Asynchronous Event is enabled */
    TCD_CFG_FILTER_gc              = (TCD_CFG_FILTER_gv << TCD_CFG_gp),  /* Input Capture Noise Cancellation Filter enabled */
    TCD_CFG_ASYNC_gc               = (TCD_CFG_ASYNC_gv << TCD_CFG_gp)   /* Asynchronous Event output qualification enabled */
} TCD_CFG_t;

/* clock select bit group values */
typedef enum TCD_CLKSEL_VALUES_enum
{
    TCD_CLKSEL_20MHZ_gv            = 0x00,  /* 20 MHz oscillator */
    TCD_CLKSEL_EXTCLK_gv           = 0x02,  /* External clock */
    TCD_CLKSEL_SYSCLK_gv           = 0x03   /* System clock */
} TCD_CLKSEL_values_t;

/* clock select bit group configurations*/
typedef enum TCD_CLKSEL_enum
{
    TCD_CLKSEL_20MHZ_gc            = (TCD_CLKSEL_20MHZ_gv << TCD_CLKSEL_gp),  /* 20 MHz oscillator */
    TCD_CLKSEL_EXTCLK_gc           = (TCD_CLKSEL_EXTCLK_gv << TCD_CLKSEL_gp),  /* External clock */
    TCD_CLKSEL_SYSCLK_gc           = (TCD_CLKSEL_SYSCLK_gv << TCD_CLKSEL_gp)   /* System clock */
} TCD_CLKSEL_t;

/* Compare C output select bit group values */
typedef enum TCD_CMPCSEL_VALUES_enum
{
    TCD_CMPCSEL_PWMA_gv            = 0x00,  /* PWM A output */
    TCD_CMPCSEL_PWMB_gv            = 0x01   /* PWM B output */
} TCD_CMPCSEL_values_t;

/* Compare C output select bit group configurations*/
typedef enum TCD_CMPCSEL_enum
{
    TCD_CMPCSEL_PWMA_gc            = (TCD_CMPCSEL_PWMA_gv << TCD_CMPCSEL_bp),  /* PWM A output */
    TCD_CMPCSEL_PWMB_gc            = (TCD_CMPCSEL_PWMB_gv << TCD_CMPCSEL_bp)   /* PWM B output */
} TCD_CMPCSEL_t;

/* Compare D output select bit group values */
typedef enum TCD_CMPDSEL_VALUES_enum
{
    TCD_CMPDSEL_PWMA_gv            = 0x00,  /* PWM A output */
    TCD_CMPDSEL_PWMB_gv            = 0x01   /* PWM B output */
} TCD_CMPDSEL_values_t;

/* Compare D output select bit group configurations*/
typedef enum TCD_CMPDSEL_enum
{
    TCD_CMPDSEL_PWMA_gc            = (TCD_CMPDSEL_PWMA_gv << TCD_CMPDSEL_bp),  /* PWM A output */
    TCD_CMPDSEL_PWMB_gc            = (TCD_CMPDSEL_PWMB_gv << TCD_CMPDSEL_bp)   /* PWM B output */
} TCD_CMPDSEL_t;

/* counter prescaler select bit group values */
typedef enum TCD_CNTPRES_VALUES_enum
{
    TCD_CNTPRES_DIV1_gv            = 0x00,  /* Sync clock divided by 1 */
    TCD_CNTPRES_DIV4_gv            = 0x01,  /* Sync clock divided by 4 */
    TCD_CNTPRES_DIV32_gv           = 0x02   /* Sync clock divided by 32 */
} TCD_CNTPRES_values_t;

/* counter prescaler select bit group configurations*/
typedef enum TCD_CNTPRES_enum
{
    TCD_CNTPRES_DIV1_gc            = (TCD_CNTPRES_DIV1_gv << TCD_CNTPRES_gp),  /* Sync clock divided by 1 */
    TCD_CNTPRES_DIV4_gc            = (TCD_CNTPRES_DIV4_gv << TCD_CNTPRES_gp),  /* Sync clock divided by 4 */
    TCD_CNTPRES_DIV32_gc           = (TCD_CNTPRES_DIV32_gv << TCD_CNTPRES_gp)   /* Sync clock divided by 32 */
} TCD_CNTPRES_t;

/* Dither select bit group values */
typedef enum TCD_DITHERSEL_VALUES_enum
{
    TCD_DITHERSEL_ONTIMEB_gv       = 0x00,  /* On-time ramp B */
    TCD_DITHERSEL_ONTIMEAB_gv      = 0x01,  /* On-time ramp A and B */
    TCD_DITHERSEL_DEADTIMEB_gv     = 0x02,  /* Dead-time rampB */
    TCD_DITHERSEL_DEADTIMEAB_gv    = 0x03   /* Dead-time ramp A and B */
} TCD_DITHERSEL_values_t;

/* Dither select bit group configurations*/
typedef enum TCD_DITHERSEL_enum
{
    TCD_DITHERSEL_ONTIMEB_gc       = (TCD_DITHERSEL_ONTIMEB_gv << TCD_DITHERSEL_gp),  /* On-time ramp B */
    TCD_DITHERSEL_ONTIMEAB_gc      = (TCD_DITHERSEL_ONTIMEAB_gv << TCD_DITHERSEL_gp),  /* On-time ramp A and B */
    TCD_DITHERSEL_DEADTIMEB_gc     = (TCD_DITHERSEL_DEADTIMEB_gv << TCD_DITHERSEL_gp),  /* Dead-time rampB */
    TCD_DITHERSEL_DEADTIMEAB_gc    = (TCD_DITHERSEL_DEADTIMEAB_gv << TCD_DITHERSEL_gp)   /* Dead-time ramp A and B */
} TCD_DITHERSEL_t;

/* Delay prescaler select bit group values */
typedef enum TCD_DLYPRESC_VALUES_enum
{
    TCD_DLYPRESC_DIV1_gv           = 0x00,  /* No prescaling */
    TCD_DLYPRESC_DIV2_gv           = 0x01,  /* Prescale with 2 */
    TCD_DLYPRESC_DIV4_gv           = 0x02,  /* Prescale with 4 */
    TCD_DLYPRESC_DIV8_gv           = 0x03   /* Prescale with 8 */
} TCD_DLYPRESC_values_t;

/* Delay prescaler select bit group configurations*/
typedef enum TCD_DLYPRESC_enum
{
    TCD_DLYPRESC_DIV1_gc           = (TCD_DLYPRESC_DIV1_gv << TCD_DLYPRESC_gp),  /* No prescaling */
    TCD_DLYPRESC_DIV2_gc           = (TCD_DLYPRESC_DIV2_gv << TCD_DLYPRESC_gp),  /* Prescale with 2 */
    TCD_DLYPRESC_DIV4_gc           = (TCD_DLYPRESC_DIV4_gv << TCD_DLYPRESC_gp),  /* Prescale with 4 */
    TCD_DLYPRESC_DIV8_gc           = (TCD_DLYPRESC_DIV8_gv << TCD_DLYPRESC_gp)   /* Prescale with 8 */
} TCD_DLYPRESC_t;

/* Delay select bit group values */
typedef enum TCD_DLYSEL_VALUES_enum
{
    TCD_DLYSEL_OFF_gv              = 0x00,  /* No delay */
    TCD_DLYSEL_INBLANK_gv          = 0x01,  /* Input blanking enabled */
    TCD_DLYSEL_EVENT_gv            = 0x02   /* Event delay enabled */
} TCD_DLYSEL_values_t;

/* Delay select bit group configurations*/
typedef enum TCD_DLYSEL_enum
{
    TCD_DLYSEL_OFF_gc              = (TCD_DLYSEL_OFF_gv << TCD_DLYSEL_gp),  /* No delay */
    TCD_DLYSEL_INBLANK_gc          = (TCD_DLYSEL_INBLANK_gv << TCD_DLYSEL_gp),  /* Input blanking enabled */
    TCD_DLYSEL_EVENT_gc            = (TCD_DLYSEL_EVENT_gv << TCD_DLYSEL_gp)   /* Event delay enabled */
} TCD_DLYSEL_t;

/* Delay trigger select bit group values */
typedef enum TCD_DLYTRIG_VALUES_enum
{
    TCD_DLYTRIG_CMPASET_gv         = 0x00,  /* Compare A set */
    TCD_DLYTRIG_CMPACLR_gv         = 0x01,  /* Compare A clear */
    TCD_DLYTRIG_CMPBSET_gv         = 0x02,  /* Compare B set */
    TCD_DLYTRIG_CMPBCLR_gv         = 0x03   /* Compare B clear */
} TCD_DLYTRIG_values_t;

/* Delay trigger select bit group configurations*/
typedef enum TCD_DLYTRIG_enum
{
    TCD_DLYTRIG_CMPASET_gc         = (TCD_DLYTRIG_CMPASET_gv << TCD_DLYTRIG_gp),  /* Compare A set */
    TCD_DLYTRIG_CMPACLR_gc         = (TCD_DLYTRIG_CMPACLR_gv << TCD_DLYTRIG_gp),  /* Compare A clear */
    TCD_DLYTRIG_CMPBSET_gc         = (TCD_DLYTRIG_CMPBSET_gv << TCD_DLYTRIG_gp),  /* Compare B set */
    TCD_DLYTRIG_CMPBCLR_gc         = (TCD_DLYTRIG_CMPBCLR_gv << TCD_DLYTRIG_gp)   /* Compare B clear */
} TCD_DLYTRIG_t;

/* Edge select bit group values */
typedef enum TCD_EDGE_VALUES_enum
{
    TCD_EDGE_FALL_LOW_gv           = 0x00,  /* The falling edge or low level of event generates retrigger or fault action */
    TCD_EDGE_RISE_HIGH_gv          = 0x01   /* The rising edge or high level of event generates retrigger or fault action */
} TCD_EDGE_values_t;

/* Edge select bit group configurations*/
typedef enum TCD_EDGE_enum
{
    TCD_EDGE_FALL_LOW_gc           = (TCD_EDGE_FALL_LOW_gv << TCD_EDGE_bp),  /* The falling edge or low level of event generates retrigger or fault action */
    TCD_EDGE_RISE_HIGH_gc          = (TCD_EDGE_RISE_HIGH_gv << TCD_EDGE_bp)   /* The rising edge or high level of event generates retrigger or fault action */
} TCD_EDGE_t;

/* Input mode select bit group values */
typedef enum TCD_INPUTMODE_VALUES_enum
{
    TCD_INPUTMODE_NONE_gv          = 0x00,  /* Input has no actions */
    TCD_INPUTMODE_JMPWAIT_gv       = 0x01,  /* Stop output, jump to opposite compare cycle and wait */
    TCD_INPUTMODE_EXECWAIT_gv      = 0x02,  /* Stop output, execute opposite compare cycle and wait */
    TCD_INPUTMODE_EXECFAULT_gv     = 0x03,  /* stop output, execute opposite compare cycle while fault active */
    TCD_INPUTMODE_FREQ_gv          = 0x04,  /* Stop all outputs, maintain frequency */
    TCD_INPUTMODE_EXECDT_gv        = 0x05,  /* Stop all outputs, execute dead time while fault active */
    TCD_INPUTMODE_WAIT_gv          = 0x06,  /* Stop all outputs, jump to next compare cycle and wait */
    TCD_INPUTMODE_WAITSW_gv        = 0x07,  /* Stop all outputs, wait for software action */
    TCD_INPUTMODE_EDGETRIG_gv      = 0x08,  /* Stop output on edge, jump to next compare cycle */
    TCD_INPUTMODE_EDGETRIGFREQ_gv  = 0x09,  /* Stop output on edge, maintain frequency */
    TCD_INPUTMODE_LVLTRIGFREQ_gv   = 0x0A   /* Stop output at level, maintain frequency */
} TCD_INPUTMODE_values_t;

/* Input mode select bit group configurations*/
typedef enum TCD_INPUTMODE_enum
{
    TCD_INPUTMODE_NONE_gc          = (TCD_INPUTMODE_NONE_gv << TCD_INPUTMODE_gp),  /* Input has no actions */
    TCD_INPUTMODE_JMPWAIT_gc       = (TCD_INPUTMODE_JMPWAIT_gv << TCD_INPUTMODE_gp),  /* Stop output, jump to opposite compare cycle and wait */
    TCD_INPUTMODE_EXECWAIT_gc      = (TCD_INPUTMODE_EXECWAIT_gv << TCD_INPUTMODE_gp),  /* Stop output, execute opposite compare cycle and wait */
    TCD_INPUTMODE_EXECFAULT_gc     = (TCD_INPUTMODE_EXECFAULT_gv << TCD_INPUTMODE_gp),  /* stop output, execute opposite compare cycle while fault active */
    TCD_INPUTMODE_FREQ_gc          = (TCD_INPUTMODE_FREQ_gv << TCD_INPUTMODE_gp),  /* Stop all outputs, maintain frequency */
    TCD_INPUTMODE_EXECDT_gc        = (TCD_INPUTMODE_EXECDT_gv << TCD_INPUTMODE_gp),  /* Stop all outputs, execute dead time while fault active */
    TCD_INPUTMODE_WAIT_gc          = (TCD_INPUTMODE_WAIT_gv << TCD_INPUTMODE_gp),  /* Stop all outputs, jump to next compare cycle and wait */
    TCD_INPUTMODE_WAITSW_gc        = (TCD_INPUTMODE_WAITSW_gv << TCD_INPUTMODE_gp),  /* Stop all outputs, wait for software action */
    TCD_INPUTMODE_EDGETRIG_gc      = (TCD_INPUTMODE_EDGETRIG_gv << TCD_INPUTMODE_gp),  /* Stop output on edge, jump to next compare cycle */
    TCD_INPUTMODE_EDGETRIGFREQ_gc  = (TCD_INPUTMODE_EDGETRIGFREQ_gv << TCD_INPUTMODE_gp),  /* Stop output on edge, maintain frequency */
    TCD_INPUTMODE_LVLTRIGFREQ_gc   = (TCD_INPUTMODE_LVLTRIGFREQ_gv << TCD_INPUTMODE_gp)   /* Stop output at level, maintain frequency */
} TCD_INPUTMODE_t;

/* Synchronization prescaler select bit group values */
typedef enum TCD_SYNCPRES_VALUES_enum
{
    TCD_SYNCPRES_DIV1_gv           = 0x00,  /* Selected clock source divided by 1 */
    TCD_SYNCPRES_DIV2_gv           = 0x01,  /* Selected clock source divided by 2 */
    TCD_SYNCPRES_DIV4_gv           = 0x02,  /* Selected clock source divided by 4 */
    TCD_SYNCPRES_DIV8_gv           = 0x03   /* Selected clock source divided by 8 */
} TCD_SYNCPRES_values_t;

/* Synchronization prescaler select bit group configurations*/
typedef enum TCD_SYNCPRES_enum
{
    TCD_SYNCPRES_DIV1_gc           = (TCD_SYNCPRES_DIV1_gv << TCD_SYNCPRES_gp),  /* Selected clock source divided by 1 */
    TCD_SYNCPRES_DIV2_gc           = (TCD_SYNCPRES_DIV2_gv << TCD_SYNCPRES_gp),  /* Selected clock source divided by 2 */
    TCD_SYNCPRES_DIV4_gc           = (TCD_SYNCPRES_DIV4_gv << TCD_SYNCPRES_gp),  /* Selected clock source divided by 4 */
    TCD_SYNCPRES_DIV8_gc           = (TCD_SYNCPRES_DIV8_gv << TCD_SYNCPRES_gp)   /* Selected clock source divided by 8 */
} TCD_SYNCPRES_t;

/* Waveform generation mode select bit group values */
typedef enum TCD_WGMODE_VALUES_enum
{
    TCD_WGMODE_ONERAMP_gv          = 0x00,  /* One ramp mode */
    TCD_WGMODE_TWORAMP_gv          = 0x01,  /* Two ramp mode */
    TCD_WGMODE_FOURRAMP_gv         = 0x02,  /* Four ramp mode */
    TCD_WGMODE_DS_gv               = 0x03   /* Dual slope mode */
} TCD_WGMODE_values_t;

/* Waveform generation mode select bit group configurations*/
typedef enum TCD_WGMODE_enum
{
    TCD_WGMODE_ONERAMP_gc          = (TCD_WGMODE_ONERAMP_gv << TCD_WGMODE_gp),  /* One ramp mode */
    TCD_WGMODE_TWORAMP_gc          = (TCD_WGMODE_TWORAMP_gv << TCD_WGMODE_gp),  /* Two ramp mode */
    TCD_WGMODE_FOURRAMP_gc         = (TCD_WGMODE_FOURRAMP_gv << TCD_WGMODE_gp),  /* Four ramp mode */
    TCD_WGMODE_DS_gc               = (TCD_WGMODE_DS_gv << TCD_WGMODE_gp)   /* Dual slope mode */
} TCD_WGMODE_t;

/* End of extract */

#endif //_XT_IO_TCD_H_
