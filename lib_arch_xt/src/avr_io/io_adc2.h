/*
 * io_adc2.h
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

#ifndef _XT_IO_ADC2_H_
#define _XT_IO_ADC2_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the ADC peripheral for the following device models:
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

/* ADC - Analog to Digital Converter */
/* ADC.CTRLA  bit masks and bit positions */
#define ADC_ENABLE_bm  0x01  /* ADC Enable bit mask. */
#define ADC_ENABLE_bp  0  /* ADC Enable bit position. */
#define ADC_LOWLAT_bm  0x20  /* ADC Low latency mode bit mask. */
#define ADC_LOWLAT_bp  5  /* ADC Low latency mode bit position. */
#define ADC_RUNSTDBY_bm  0x80  /* Run standby mode bit mask. */
#define ADC_RUNSTDBY_bp  7  /* Run standby mode bit position. */

/* ADC.CTRLB  bit masks and bit positions */
#define ADC_PRESC_gm  0x0F  /* Prescaler Value group mask. */
#define ADC_PRESC_gp  0  /* Prescaler Value group position. */
#define ADC_PRESC_0_bm  (1<<0)  /* Prescaler Value bit 0 mask. */
#define ADC_PRESC_0_bp  0  /* Prescaler Value bit 0 position. */
#define ADC_PRESC_1_bm  (1<<1)  /* Prescaler Value bit 1 mask. */
#define ADC_PRESC_1_bp  1  /* Prescaler Value bit 1 position. */
#define ADC_PRESC_2_bm  (1<<2)  /* Prescaler Value bit 2 mask. */
#define ADC_PRESC_2_bp  2  /* Prescaler Value bit 2 position. */
#define ADC_PRESC_3_bm  (1<<3)  /* Prescaler Value bit 3 mask. */
#define ADC_PRESC_3_bp  3  /* Prescaler Value bit 3 position. */

/* ADC.CTRLC  bit masks and bit positions */
#define ADC_REFSEL_gm  0x07  /* Reference select group mask. */
#define ADC_REFSEL_gp  0  /* Reference select group position. */
#define ADC_REFSEL_0_bm  (1<<0)  /* Reference select bit 0 mask. */
#define ADC_REFSEL_0_bp  0  /* Reference select bit 0 position. */
#define ADC_REFSEL_1_bm  (1<<1)  /* Reference select bit 1 mask. */
#define ADC_REFSEL_1_bp  1  /* Reference select bit 1 position. */
#define ADC_REFSEL_2_bm  (1<<2)  /* Reference select bit 2 mask. */
#define ADC_REFSEL_2_bp  2  /* Reference select bit 2 position. */
#define ADC_TIMEBASE_gm  0xF8  /* Reference Selection group mask. */
#define ADC_TIMEBASE_gp  3  /* Reference Selection group position. */
#define ADC_TIMEBASE_0_bm  (1<<3)  /* Reference Selection bit 0 mask. */
#define ADC_TIMEBASE_0_bp  3  /* Reference Selection bit 0 position. */
#define ADC_TIMEBASE_1_bm  (1<<4)  /* Reference Selection bit 1 mask. */
#define ADC_TIMEBASE_1_bp  4  /* Reference Selection bit 1 position. */
#define ADC_TIMEBASE_2_bm  (1<<5)  /* Reference Selection bit 2 mask. */
#define ADC_TIMEBASE_2_bp  5  /* Reference Selection bit 2 position. */
#define ADC_TIMEBASE_3_bm  (1<<6)  /* Reference Selection bit 3 mask. */
#define ADC_TIMEBASE_3_bp  6  /* Reference Selection bit 3 position. */
#define ADC_TIMEBASE_4_bm  (1<<7)  /* Reference Selection bit 4 mask. */
#define ADC_TIMEBASE_4_bp  7  /* Reference Selection bit 4 position. */

/* ADC.CTRLD  bit masks and bit positions */
#define ADC_WINCM_gm  0x07  /* Window Comparator Mode group mask. */
#define ADC_WINCM_gp  0  /* Window Comparator Mode group position. */
#define ADC_WINCM_0_bm  (1<<0)  /* Window Comparator Mode bit 0 mask. */
#define ADC_WINCM_0_bp  0  /* Window Comparator Mode bit 0 position. */
#define ADC_WINCM_1_bm  (1<<1)  /* Window Comparator Mode bit 1 mask. */
#define ADC_WINCM_1_bp  1  /* Window Comparator Mode bit 1 position. */
#define ADC_WINCM_2_bm  (1<<2)  /* Window Comparator Mode bit 2 mask. */
#define ADC_WINCM_2_bp  2  /* Window Comparator Mode bit 2 position. */
#define ADC_WINSRC_bm  0x08  /* Window Mode Source bit mask. */
#define ADC_WINSRC_bp  3  /* Window Mode Source bit position. */

/* ADC.INTCTRL  bit masks and bit positions */
#define ADC_RESRDY_bm  0x01  /* Result Ready Interrupt Enable bit mask. */
#define ADC_RESRDY_bp  0  /* Result Ready Interrupt Enable bit position. */
#define ADC_SAMPRDY_bm  0x02  /* Sample Ready Interrupt Enable bit mask. */
#define ADC_SAMPRDY_bp  1  /* Sample Ready Interrupt Enable bit position. */
#define ADC_WCMP_bm  0x04  /* Window Comparator Interrupt Enable bit mask. */
#define ADC_WCMP_bp  2  /* Window Comparator Interrupt Enable bit position. */
#define ADC_RESOVR_bm  0x08  /* Result Overwritten Interrupt Enable bit mask. */
#define ADC_RESOVR_bp  3  /* Result Overwritten Interrupt Enable bit position. */
#define ADC_SAMPOVR_bm  0x10  /* Sample Overwritten Interrupt Enable bit mask. */
#define ADC_SAMPOVR_bp  4  /* Sample Overwritten Interrupt Enable bit position. */
#define ADC_TRIGOVR_bm  0x20  /* Trigger Overrun Interrupt Enable bit mask. */
#define ADC_TRIGOVR_bp  5  /* Trigger Overrun Interrupt Enable bit position. */

/* ADC.INTFLAGS  bit masks and bit positions */
/* ADC_RESRDY  is already defined. */
/* ADC_SAMPRDY  is already defined. */
/* ADC_WCMP  is already defined. */
/* ADC_RESOVR  is already defined. */
/* ADC_SAMPOVR  is already defined. */
/* ADC_TRIGOVR  is already defined. */

/* ADC.STATUS  bit masks and bit positions */
#define ADC_ADCBUSY_bm  0x01  /* ADC Busy bit mask. */
#define ADC_ADCBUSY_bp  0  /* ADC Busy bit position. */

/* ADC.DBGCTRL  bit masks and bit positions */
#define ADC_DBGRUN_bm  0x01  /* Debug run bit mask. */
#define ADC_DBGRUN_bp  0  /* Debug run bit position. */

/* ADC.CTRLE  bit masks and bit positions */
#define ADC_SAMPDUR_gm  0xFF  /* Sampling time group mask. */
#define ADC_SAMPDUR_gp  0  /* Sampling time group position. */
#define ADC_SAMPDUR_0_bm  (1<<0)  /* Sampling time bit 0 mask. */
#define ADC_SAMPDUR_0_bp  0  /* Sampling time bit 0 position. */
#define ADC_SAMPDUR_1_bm  (1<<1)  /* Sampling time bit 1 mask. */
#define ADC_SAMPDUR_1_bp  1  /* Sampling time bit 1 position. */
#define ADC_SAMPDUR_2_bm  (1<<2)  /* Sampling time bit 2 mask. */
#define ADC_SAMPDUR_2_bp  2  /* Sampling time bit 2 position. */
#define ADC_SAMPDUR_3_bm  (1<<3)  /* Sampling time bit 3 mask. */
#define ADC_SAMPDUR_3_bp  3  /* Sampling time bit 3 position. */
#define ADC_SAMPDUR_4_bm  (1<<4)  /* Sampling time bit 4 mask. */
#define ADC_SAMPDUR_4_bp  4  /* Sampling time bit 4 position. */
#define ADC_SAMPDUR_5_bm  (1<<5)  /* Sampling time bit 5 mask. */
#define ADC_SAMPDUR_5_bp  5  /* Sampling time bit 5 position. */
#define ADC_SAMPDUR_6_bm  (1<<6)  /* Sampling time bit 6 mask. */
#define ADC_SAMPDUR_6_bp  6  /* Sampling time bit 6 position. */
#define ADC_SAMPDUR_7_bm  (1<<7)  /* Sampling time bit 7 mask. */
#define ADC_SAMPDUR_7_bp  7  /* Sampling time bit 7 position. */

/* ADC.CTRLF  bit masks and bit positions */
#define ADC_SAMPNUM_gm  0x0F  /* Sample numbers group mask. */
#define ADC_SAMPNUM_gp  0  /* Sample numbers group position. */
#define ADC_SAMPNUM_0_bm  (1<<0)  /* Sample numbers bit 0 mask. */
#define ADC_SAMPNUM_0_bp  0  /* Sample numbers bit 0 position. */
#define ADC_SAMPNUM_1_bm  (1<<1)  /* Sample numbers bit 1 mask. */
#define ADC_SAMPNUM_1_bp  1  /* Sample numbers bit 1 position. */
#define ADC_SAMPNUM_2_bm  (1<<2)  /* Sample numbers bit 2 mask. */
#define ADC_SAMPNUM_2_bp  2  /* Sample numbers bit 2 position. */
#define ADC_SAMPNUM_3_bm  (1<<3)  /* Sample numbers bit 3 mask. */
#define ADC_SAMPNUM_3_bp  3  /* Sample numbers bit 3 position. */
#define ADC_LEFTADJ_bm  0x10  /* Left adjust bit mask. */
#define ADC_LEFTADJ_bp  4  /* Left adjust bit position. */
#define ADC_FREERUN_bm  0x20  /* Free running mode bit mask. */
#define ADC_FREERUN_bp  5  /* Free running mode bit position. */

/* ADC.COMMAND  bit masks and bit positions */
#define ADC_START_gm  0x07  /* Start command group mask. */
#define ADC_START_gp  0  /* Start command group position. */
#define ADC_START_0_bm  (1<<0)  /* Start command bit 0 mask. */
#define ADC_START_0_bp  0  /* Start command bit 0 position. */
#define ADC_START_1_bm  (1<<1)  /* Start command bit 1 mask. */
#define ADC_START_1_bp  1  /* Start command bit 1 position. */
#define ADC_START_2_bm  (1<<2)  /* Start command bit 2 mask. */
#define ADC_START_2_bp  2  /* Start command bit 2 position. */
#define ADC_MODE_gm  0x70  /* Mode group mask. */
#define ADC_MODE_gp  4  /* Mode group position. */
#define ADC_MODE_0_bm  (1<<4)  /* Mode bit 0 mask. */
#define ADC_MODE_0_bp  4  /* Mode bit 0 position. */
#define ADC_MODE_1_bm  (1<<5)  /* Mode bit 1 mask. */
#define ADC_MODE_1_bp  5  /* Mode bit 1 position. */
#define ADC_MODE_2_bm  (1<<6)  /* Mode bit 2 mask. */
#define ADC_MODE_2_bp  6  /* Mode bit 2 position. */
#define ADC_DIFF_bm  0x80  /* Differential mode bit mask. */
#define ADC_DIFF_bp  7  /* Differential mode bit position. */

/* ADC.PGACTRL  bit masks and bit positions */
#define ADC_PGAEN_bm  0x01  /* PGA Enable bit mask. */
#define ADC_PGAEN_bp  0  /* PGA Enable bit position. */
#define ADC_ADCPGASAMPDUR_gm  0x06  /* ADC PGA Sample Duration group mask. */
#define ADC_ADCPGASAMPDUR_gp  1  /* ADC PGA Sample Duration group position. */
#define ADC_ADCPGASAMPDUR_0_bm  (1<<1)  /* ADC PGA Sample Duration bit 0 mask. */
#define ADC_ADCPGASAMPDUR_0_bp  1  /* ADC PGA Sample Duration bit 0 position. */
#define ADC_ADCPGASAMPDUR_1_bm  (1<<2)  /* ADC PGA Sample Duration bit 1 mask. */
#define ADC_ADCPGASAMPDUR_1_bp  2  /* ADC PGA Sample Duration bit 1 position. */
#define ADC_PGABIASSEL_gm  0x18  /* PGA BIAS Select group mask. */
#define ADC_PGABIASSEL_gp  3  /* PGA BIAS Select group position. */
#define ADC_PGABIASSEL_0_bm  (1<<3)  /* PGA BIAS Select bit 0 mask. */
#define ADC_PGABIASSEL_0_bp  3  /* PGA BIAS Select bit 0 position. */
#define ADC_PGABIASSEL_1_bm  (1<<4)  /* PGA BIAS Select bit 1 mask. */
#define ADC_PGABIASSEL_1_bp  4  /* PGA BIAS Select bit 1 position. */
#define ADC_GAIN_gm  0xE0  /* Gain group mask. */
#define ADC_GAIN_gp  5  /* Gain group position. */
#define ADC_GAIN_0_bm  (1<<5)  /* Gain bit 0 mask. */
#define ADC_GAIN_0_bp  5  /* Gain bit 0 position. */
#define ADC_GAIN_1_bm  (1<<6)  /* Gain bit 1 mask. */
#define ADC_GAIN_1_bp  6  /* Gain bit 1 position. */
#define ADC_GAIN_2_bm  (1<<7)  /* Gain bit 2 mask. */
#define ADC_GAIN_2_bp  7  /* Gain bit 2 position. */

/* ADC.MUXPOS  bit masks and bit positions */
#define ADC_MUXPOS_gm  0x3F  /* Analog Channel Selection Bits group mask. */
#define ADC_MUXPOS_gp  0  /* Analog Channel Selection Bits group position. */
#define ADC_MUXPOS_0_bm  (1<<0)  /* Analog Channel Selection Bits bit 0 mask. */
#define ADC_MUXPOS_0_bp  0  /* Analog Channel Selection Bits bit 0 position. */
#define ADC_MUXPOS_1_bm  (1<<1)  /* Analog Channel Selection Bits bit 1 mask. */
#define ADC_MUXPOS_1_bp  1  /* Analog Channel Selection Bits bit 1 position. */
#define ADC_MUXPOS_2_bm  (1<<2)  /* Analog Channel Selection Bits bit 2 mask. */
#define ADC_MUXPOS_2_bp  2  /* Analog Channel Selection Bits bit 2 position. */
#define ADC_MUXPOS_3_bm  (1<<3)  /* Analog Channel Selection Bits bit 3 mask. */
#define ADC_MUXPOS_3_bp  3  /* Analog Channel Selection Bits bit 3 position. */
#define ADC_MUXPOS_4_bm  (1<<4)  /* Analog Channel Selection Bits bit 4 mask. */
#define ADC_MUXPOS_4_bp  4  /* Analog Channel Selection Bits bit 4 position. */
#define ADC_MUXPOS_5_bm  (1<<5)  /* Analog Channel Selection Bits bit 5 mask. */
#define ADC_MUXPOS_5_bp  5  /* Analog Channel Selection Bits bit 5 position. */
#define ADC_VIA_gm  0xC0  /* VIA group mask. */
#define ADC_VIA_gp  6  /* VIA group position. */
#define ADC_VIA_0_bm  (1<<6)  /* VIA bit 0 mask. */
#define ADC_VIA_0_bp  6  /* VIA bit 0 position. */
#define ADC_VIA_1_bm  (1<<7)  /* VIA bit 1 mask. */
#define ADC_VIA_1_bp  7  /* VIA bit 1 position. */

/* ADC.MUXNEG  bit masks and bit positions */
#define ADC_MUXNEG_gm  0x3F  /* Analog Channel Selection Bits group mask. */
#define ADC_MUXNEG_gp  0  /* Analog Channel Selection Bits group position. */
#define ADC_MUXNEG_0_bm  (1<<0)  /* Analog Channel Selection Bits bit 0 mask. */
#define ADC_MUXNEG_0_bp  0  /* Analog Channel Selection Bits bit 0 position. */
#define ADC_MUXNEG_1_bm  (1<<1)  /* Analog Channel Selection Bits bit 1 mask. */
#define ADC_MUXNEG_1_bp  1  /* Analog Channel Selection Bits bit 1 position. */
#define ADC_MUXNEG_2_bm  (1<<2)  /* Analog Channel Selection Bits bit 2 mask. */
#define ADC_MUXNEG_2_bp  2  /* Analog Channel Selection Bits bit 2 position. */
#define ADC_MUXNEG_3_bm  (1<<3)  /* Analog Channel Selection Bits bit 3 mask. */
#define ADC_MUXNEG_3_bp  3  /* Analog Channel Selection Bits bit 3 position. */
#define ADC_MUXNEG_4_bm  (1<<4)  /* Analog Channel Selection Bits bit 4 mask. */
#define ADC_MUXNEG_4_bp  4  /* Analog Channel Selection Bits bit 4 position. */
#define ADC_MUXNEG_5_bm  (1<<5)  /* Analog Channel Selection Bits bit 5 mask. */
#define ADC_MUXNEG_5_bp  5  /* Analog Channel Selection Bits bit 5 position. */
/* ADC_VIA  is already defined. */

/* ADC.TEMP0  bit masks and bit positions */
#define ADC_TEMP_gm  0xFF  /* Temporary group mask. */
#define ADC_TEMP_gp  0  /* Temporary group position. */
#define ADC_TEMP_0_bm  (1<<0)  /* Temporary bit 0 mask. */
#define ADC_TEMP_0_bp  0  /* Temporary bit 0 position. */
#define ADC_TEMP_1_bm  (1<<1)  /* Temporary bit 1 mask. */
#define ADC_TEMP_1_bp  1  /* Temporary bit 1 position. */
#define ADC_TEMP_2_bm  (1<<2)  /* Temporary bit 2 mask. */
#define ADC_TEMP_2_bp  2  /* Temporary bit 2 position. */
#define ADC_TEMP_3_bm  (1<<3)  /* Temporary bit 3 mask. */
#define ADC_TEMP_3_bp  3  /* Temporary bit 3 position. */
#define ADC_TEMP_4_bm  (1<<4)  /* Temporary bit 4 mask. */
#define ADC_TEMP_4_bp  4  /* Temporary bit 4 position. */
#define ADC_TEMP_5_bm  (1<<5)  /* Temporary bit 5 mask. */
#define ADC_TEMP_5_bp  5  /* Temporary bit 5 position. */
#define ADC_TEMP_6_bm  (1<<6)  /* Temporary bit 6 mask. */
#define ADC_TEMP_6_bp  6  /* Temporary bit 6 position. */
#define ADC_TEMP_7_bm  (1<<7)  /* Temporary bit 7 mask. */
#define ADC_TEMP_7_bp  7  /* Temporary bit 7 position. */

/* ADC.TEMP1  bit masks and bit positions */
/* ADC_TEMP  is already defined. */

/* ADC.TEMP2  bit masks and bit positions */
/* ADC_TEMP  is already defined. */


/* Analog to Digital Converter */
typedef struct ADC_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t CTRLC;  /* Control C */
    register8_t CTRLD;  /* Control D */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t STATUS;  /* Status register */
    register8_t DBGCTRL;  /* Debug Control */
    register8_t CTRLE;  /* Control E */
    register8_t CTRLF;  /* Control F */
    register8_t COMMAND;  /* Command register */
    register8_t PGACTRL;  /* PGA Control */
    register8_t MUXPOS;  /* Positive mux input */
    register8_t MUXNEG;  /* Negative mux input */
    register8_t reserved_1[2];
    _DWORDREGISTER(RESULT);  /* Result */
    _WORDREGISTER(SAMPLE);  /* Sample */
    register8_t reserved_2[2];
    register8_t TEMP0;  /* Temporary Data 0 */
    register8_t TEMP1;  /* Temporary Data 1 */
    register8_t TEMP2;  /* Temporary Data 2 */
    register8_t reserved_3[1];
    _WORDREGISTER(WINLT);  /* Window Low Threshold */
    _WORDREGISTER(WINHT);  /* Window High Threshold */
    register8_t reserved_4[32];
} ADC_t;

/* ADC PGA Sample Duration select bit group values */
typedef enum ADC_ADCPGASAMPDUR_VALUES_enum
{
    ADC_ADCPGASAMPDUR_6CLK_gv      = 0x00,  /* 6 ADC cycles */
    ADC_ADCPGASAMPDUR_15CLK_gv     = 0x01,  /* 15 ADC cycles */
    ADC_ADCPGASAMPDUR_20CLK_gv     = 0x02   /* 20 ADC cycles */
} ADC_ADCPGASAMPDUR_values_t;

/* ADC PGA Sample Duration select bit group configurations*/
typedef enum ADC_ADCPGASAMPDUR_enum
{
    ADC_ADCPGASAMPDUR_6CLK_gc      = (ADC_ADCPGASAMPDUR_6CLK_gv << ADC_ADCPGASAMPDUR_gp),  /* 6 ADC cycles */
    ADC_ADCPGASAMPDUR_15CLK_gc     = (ADC_ADCPGASAMPDUR_15CLK_gv << ADC_ADCPGASAMPDUR_gp),  /* 15 ADC cycles */
    ADC_ADCPGASAMPDUR_20CLK_gc     = (ADC_ADCPGASAMPDUR_20CLK_gv << ADC_ADCPGASAMPDUR_gp)   /* 20 ADC cycles */
} ADC_ADCPGASAMPDUR_t;

/* Gain select bit group values */
typedef enum ADC_GAIN_VALUES_enum
{
    ADC_GAIN_1X_gv                 = 0x00,  /* 1X Gain */
    ADC_GAIN_2X_gv                 = 0x01,  /* 2X Gain */
    ADC_GAIN_4X_gv                 = 0x02,  /* 4X Gain */
    ADC_GAIN_8X_gv                 = 0x03,  /* 8X Gain */
    ADC_GAIN_16X_gv                = 0x04   /* 16X Gain */
} ADC_GAIN_values_t;

/* Gain select bit group configurations*/
typedef enum ADC_GAIN_enum
{
    ADC_GAIN_1X_gc                 = (ADC_GAIN_1X_gv << ADC_GAIN_gp),  /* 1X Gain */
    ADC_GAIN_2X_gc                 = (ADC_GAIN_2X_gv << ADC_GAIN_gp),  /* 2X Gain */
    ADC_GAIN_4X_gc                 = (ADC_GAIN_4X_gv << ADC_GAIN_gp),  /* 4X Gain */
    ADC_GAIN_8X_gc                 = (ADC_GAIN_8X_gv << ADC_GAIN_gp),  /* 8X Gain */
    ADC_GAIN_16X_gc                = (ADC_GAIN_16X_gv << ADC_GAIN_gp)   /* 16X Gain */
} ADC_GAIN_t;

/* Mode select bit group values */
typedef enum ADC_MODE_VALUES_enum
{
    ADC_MODE_SINGLE_8BIT_gv        = 0x00,  /* Single Conversion 8-bit */
    ADC_MODE_SINGLE_12BIT_gv       = 0x01,  /* Single Conversion 12-bit */
    ADC_MODE_SERIES_gv             = 0x02,  /* Series Accumulation */
    ADC_MODE_SERIES_SCALING_gv     = 0x03,  /* Series Accumulation with Scaling */
    ADC_MODE_BURST_gv              = 0x04,  /* Burst Accumulation */
    ADC_MODE_BURST_SCALING_gv      = 0x05   /* Burst Accumulation with Scaling */
} ADC_MODE_values_t;

/* Mode select bit group configurations*/
typedef enum ADC_MODE_enum
{
    ADC_MODE_SINGLE_8BIT_gc        = (ADC_MODE_SINGLE_8BIT_gv << ADC_MODE_gp),  /* Single Conversion 8-bit */
    ADC_MODE_SINGLE_12BIT_gc       = (ADC_MODE_SINGLE_12BIT_gv << ADC_MODE_gp),  /* Single Conversion 12-bit */
    ADC_MODE_SERIES_gc             = (ADC_MODE_SERIES_gv << ADC_MODE_gp),  /* Series Accumulation */
    ADC_MODE_SERIES_SCALING_gc     = (ADC_MODE_SERIES_SCALING_gv << ADC_MODE_gp),  /* Series Accumulation with Scaling */
    ADC_MODE_BURST_gc              = (ADC_MODE_BURST_gv << ADC_MODE_gp),  /* Burst Accumulation */
    ADC_MODE_BURST_SCALING_gc      = (ADC_MODE_BURST_SCALING_gv << ADC_MODE_gp)   /* Burst Accumulation with Scaling */
} ADC_MODE_t;

/* Analog Channel Selection Bits bit group values */
typedef enum ADC_MUXNEG_VALUES_enum
{
    ADC_MUXNEG_AIN1_gv             = 0x01,  /* ADC input pin 1 */
    ADC_MUXNEG_AIN2_gv             = 0x02,  /* ADC input pin 2 */
    ADC_MUXNEG_AIN3_gv             = 0x03,  /* ADC input pin 3 */
    ADC_MUXNEG_AIN4_gv             = 0x04,  /* ADC input pin 4 */
    ADC_MUXNEG_AIN5_gv             = 0x05,  /* ADC input pin 5 */
    ADC_MUXNEG_AIN6_gv             = 0x06,  /* ADC input pin 6 */
    ADC_MUXNEG_AIN7_gv             = 0x07,  /* ADC input pin 7 */
    ADC_MUXNEG_GND_gv              = 0x30,  /* Ground */
    ADC_MUXNEG_VDDDIV10_gv         = 0x31,  /* VDD/10 */
    ADC_MUXNEG_DACREF0_gv          = 0x33   /* DACREF from AC0 */
} ADC_MUXNEG_values_t;

/* Analog Channel Selection Bits bit group configurations*/
typedef enum ADC_MUXNEG_enum
{
    ADC_MUXNEG_AIN1_gc             = (ADC_MUXNEG_AIN1_gv << ADC_MUXNEG_gp),  /* ADC input pin 1 */
    ADC_MUXNEG_AIN2_gc             = (ADC_MUXNEG_AIN2_gv << ADC_MUXNEG_gp),  /* ADC input pin 2 */
    ADC_MUXNEG_AIN3_gc             = (ADC_MUXNEG_AIN3_gv << ADC_MUXNEG_gp),  /* ADC input pin 3 */
    ADC_MUXNEG_AIN4_gc             = (ADC_MUXNEG_AIN4_gv << ADC_MUXNEG_gp),  /* ADC input pin 4 */
    ADC_MUXNEG_AIN5_gc             = (ADC_MUXNEG_AIN5_gv << ADC_MUXNEG_gp),  /* ADC input pin 5 */
    ADC_MUXNEG_AIN6_gc             = (ADC_MUXNEG_AIN6_gv << ADC_MUXNEG_gp),  /* ADC input pin 6 */
    ADC_MUXNEG_AIN7_gc             = (ADC_MUXNEG_AIN7_gv << ADC_MUXNEG_gp),  /* ADC input pin 7 */
    ADC_MUXNEG_GND_gc              = (ADC_MUXNEG_GND_gv << ADC_MUXNEG_gp),  /* Ground */
    ADC_MUXNEG_VDDDIV10_gc         = (ADC_MUXNEG_VDDDIV10_gv << ADC_MUXNEG_gp),  /* VDD/10 */
    ADC_MUXNEG_DACREF0_gc          = (ADC_MUXNEG_DACREF0_gv << ADC_MUXNEG_gp)   /* DACREF from AC0 */
} ADC_MUXNEG_t;

/* Analog Channel Selection Bits bit group values */
typedef enum ADC_MUXPOS_VALUES_enum
{
    ADC_MUXPOS_AIN1_gv             = 0x01,  /* ADC input pin 1 */
    ADC_MUXPOS_AIN2_gv             = 0x02,  /* ADC input pin 2 */
    ADC_MUXPOS_AIN3_gv             = 0x03,  /* ADC input pin 3 */
    ADC_MUXPOS_AIN4_gv             = 0x04,  /* ADC input pin 4 */
    ADC_MUXPOS_AIN5_gv             = 0x05,  /* ADC input pin 5 */
    ADC_MUXPOS_AIN6_gv             = 0x06,  /* ADC input pin 6 */
    ADC_MUXPOS_AIN7_gv             = 0x07,  /* ADC input pin 7 */
    ADC_MUXPOS_AIN8_gv             = 0x08,  /* ADC input pin 8 */
    ADC_MUXPOS_AIN9_gv             = 0x09,  /* ADC input pin 9 */
    ADC_MUXPOS_AIN10_gv            = 0x0A,  /* ADC input pin 10 */
    ADC_MUXPOS_AIN11_gv            = 0x0B,  /* ADC input pin 11 */
    ADC_MUXPOS_AIN12_gv            = 0x0C,  /* ADC input pin 12 */
    ADC_MUXPOS_AIN13_gv            = 0x0D,  /* ADC input pin 13 */
    ADC_MUXPOS_AIN14_gv            = 0x0E,  /* ADC input pin 14 */
    ADC_MUXPOS_AIN15_gv            = 0x0F,  /* ADC input pin 15 */
    ADC_MUXPOS_GND_gv              = 0x30,  /* Ground */
    ADC_MUXPOS_VDDDIV10_gv         = 0x31,  /* VDD/10 */
    ADC_MUXPOS_TEMPSENSE_gv        = 0x32,  /* Temperature sensor */
    ADC_MUXPOS_DACREF0_gv          = 0x33   /* DACREF from AC0 */
} ADC_MUXPOS_values_t;

/* Analog Channel Selection Bits bit group configurations*/
typedef enum ADC_MUXPOS_enum
{
    ADC_MUXPOS_AIN1_gc             = (ADC_MUXPOS_AIN1_gv << ADC_MUXPOS_gp),  /* ADC input pin 1 */
    ADC_MUXPOS_AIN2_gc             = (ADC_MUXPOS_AIN2_gv << ADC_MUXPOS_gp),  /* ADC input pin 2 */
    ADC_MUXPOS_AIN3_gc             = (ADC_MUXPOS_AIN3_gv << ADC_MUXPOS_gp),  /* ADC input pin 3 */
    ADC_MUXPOS_AIN4_gc             = (ADC_MUXPOS_AIN4_gv << ADC_MUXPOS_gp),  /* ADC input pin 4 */
    ADC_MUXPOS_AIN5_gc             = (ADC_MUXPOS_AIN5_gv << ADC_MUXPOS_gp),  /* ADC input pin 5 */
    ADC_MUXPOS_AIN6_gc             = (ADC_MUXPOS_AIN6_gv << ADC_MUXPOS_gp),  /* ADC input pin 6 */
    ADC_MUXPOS_AIN7_gc             = (ADC_MUXPOS_AIN7_gv << ADC_MUXPOS_gp),  /* ADC input pin 7 */
    ADC_MUXPOS_AIN8_gc             = (ADC_MUXPOS_AIN8_gv << ADC_MUXPOS_gp),  /* ADC input pin 8 */
    ADC_MUXPOS_AIN9_gc             = (ADC_MUXPOS_AIN9_gv << ADC_MUXPOS_gp),  /* ADC input pin 9 */
    ADC_MUXPOS_AIN10_gc            = (ADC_MUXPOS_AIN10_gv << ADC_MUXPOS_gp),  /* ADC input pin 10 */
    ADC_MUXPOS_AIN11_gc            = (ADC_MUXPOS_AIN11_gv << ADC_MUXPOS_gp),  /* ADC input pin 11 */
    ADC_MUXPOS_AIN12_gc            = (ADC_MUXPOS_AIN12_gv << ADC_MUXPOS_gp),  /* ADC input pin 12 */
    ADC_MUXPOS_AIN13_gc            = (ADC_MUXPOS_AIN13_gv << ADC_MUXPOS_gp),  /* ADC input pin 13 */
    ADC_MUXPOS_AIN14_gc            = (ADC_MUXPOS_AIN14_gv << ADC_MUXPOS_gp),  /* ADC input pin 14 */
    ADC_MUXPOS_AIN15_gc            = (ADC_MUXPOS_AIN15_gv << ADC_MUXPOS_gp),  /* ADC input pin 15 */
    ADC_MUXPOS_GND_gc              = (ADC_MUXPOS_GND_gv << ADC_MUXPOS_gp),  /* Ground */
    ADC_MUXPOS_VDDDIV10_gc         = (ADC_MUXPOS_VDDDIV10_gv << ADC_MUXPOS_gp),  /* VDD/10 */
    ADC_MUXPOS_TEMPSENSE_gc        = (ADC_MUXPOS_TEMPSENSE_gv << ADC_MUXPOS_gp),  /* Temperature sensor */
    ADC_MUXPOS_DACREF0_gc          = (ADC_MUXPOS_DACREF0_gv << ADC_MUXPOS_gp)   /* DACREF from AC0 */
} ADC_MUXPOS_t;

/* PGA BIAS Select bit group values */
typedef enum ADC_PGABIASSEL_VALUES_enum
{
    ADC_PGABIASSEL_1X_gv           = 0x00,  /* 1x BIAS current */
    ADC_PGABIASSEL_3_4X_gv         = 0x01,  /* 3/4x BIAS current */
    ADC_PGABIASSEL_1_2X_gv         = 0x02,  /* 1/2x BIAS current */
    ADC_PGABIASSEL_1_4X_gv         = 0x03   /* 1/4x BIAS current */
} ADC_PGABIASSEL_values_t;

/* PGA BIAS Select bit group configurations*/
typedef enum ADC_PGABIASSEL_enum
{
    ADC_PGABIASSEL_1X_gc           = (ADC_PGABIASSEL_1X_gv << ADC_PGABIASSEL_gp),  /* 1x BIAS current */
    ADC_PGABIASSEL_3_4X_gc         = (ADC_PGABIASSEL_3_4X_gv << ADC_PGABIASSEL_gp),  /* 3/4x BIAS current */
    ADC_PGABIASSEL_1_2X_gc         = (ADC_PGABIASSEL_1_2X_gv << ADC_PGABIASSEL_gp),  /* 1/2x BIAS current */
    ADC_PGABIASSEL_1_4X_gc         = (ADC_PGABIASSEL_1_4X_gv << ADC_PGABIASSEL_gp)   /* 1/4x BIAS current */
} ADC_PGABIASSEL_t;

/* Prescaler Value select bit group values */
typedef enum ADC_PRESC_VALUES_enum
{
    ADC_PRESC_DIV2_gv              = 0x00,  /* System clock divided by 2 */
    ADC_PRESC_DIV4_gv              = 0x01,  /* System clock divided by 4 */
    ADC_PRESC_DIV6_gv              = 0x02,  /* System clock divided by 6 */
    ADC_PRESC_DIV8_gv              = 0x03,  /* System clock divided by 8 */
    ADC_PRESC_DIV10_gv             = 0x04,  /* System clock divided by 10 */
    ADC_PRESC_DIV12_gv             = 0x05,  /* System clock divided by 12 */
    ADC_PRESC_DIV14_gv             = 0x06,  /* System clock divided by 14 */
    ADC_PRESC_DIV16_gv             = 0x07,  /* System clock divided by 16 */
    ADC_PRESC_DIV20_gv             = 0x08,  /* System clock divided by 20 */
    ADC_PRESC_DIV24_gv             = 0x09,  /* System clock divided by 24 */
    ADC_PRESC_DIV28_gv             = 0x0A,  /* System clock divided by 28 */
    ADC_PRESC_DIV32_gv             = 0x0B,  /* System clock divided by 32 */
    ADC_PRESC_DIV40_gv             = 0x0C,  /* System clock divided by 40 */
    ADC_PRESC_DIV48_gv             = 0x0D,  /* System clock divided by 48 */
    ADC_PRESC_DIV56_gv             = 0x0E,  /* System clock divided by 56 */
    ADC_PRESC_DIV64_gv             = 0x0F   /* System clock divided by 64 */
} ADC_PRESC_values_t;

/* Prescaler Value select bit group configurations*/
typedef enum ADC_PRESC_enum
{
    ADC_PRESC_DIV2_gc              = (ADC_PRESC_DIV2_gv << ADC_PRESC_gp),  /* System clock divided by 2 */
    ADC_PRESC_DIV4_gc              = (ADC_PRESC_DIV4_gv << ADC_PRESC_gp),  /* System clock divided by 4 */
    ADC_PRESC_DIV6_gc              = (ADC_PRESC_DIV6_gv << ADC_PRESC_gp),  /* System clock divided by 6 */
    ADC_PRESC_DIV8_gc              = (ADC_PRESC_DIV8_gv << ADC_PRESC_gp),  /* System clock divided by 8 */
    ADC_PRESC_DIV10_gc             = (ADC_PRESC_DIV10_gv << ADC_PRESC_gp),  /* System clock divided by 10 */
    ADC_PRESC_DIV12_gc             = (ADC_PRESC_DIV12_gv << ADC_PRESC_gp),  /* System clock divided by 12 */
    ADC_PRESC_DIV14_gc             = (ADC_PRESC_DIV14_gv << ADC_PRESC_gp),  /* System clock divided by 14 */
    ADC_PRESC_DIV16_gc             = (ADC_PRESC_DIV16_gv << ADC_PRESC_gp),  /* System clock divided by 16 */
    ADC_PRESC_DIV20_gc             = (ADC_PRESC_DIV20_gv << ADC_PRESC_gp),  /* System clock divided by 20 */
    ADC_PRESC_DIV24_gc             = (ADC_PRESC_DIV24_gv << ADC_PRESC_gp),  /* System clock divided by 24 */
    ADC_PRESC_DIV28_gc             = (ADC_PRESC_DIV28_gv << ADC_PRESC_gp),  /* System clock divided by 28 */
    ADC_PRESC_DIV32_gc             = (ADC_PRESC_DIV32_gv << ADC_PRESC_gp),  /* System clock divided by 32 */
    ADC_PRESC_DIV40_gc             = (ADC_PRESC_DIV40_gv << ADC_PRESC_gp),  /* System clock divided by 40 */
    ADC_PRESC_DIV48_gc             = (ADC_PRESC_DIV48_gv << ADC_PRESC_gp),  /* System clock divided by 48 */
    ADC_PRESC_DIV56_gc             = (ADC_PRESC_DIV56_gv << ADC_PRESC_gp),  /* System clock divided by 56 */
    ADC_PRESC_DIV64_gc             = (ADC_PRESC_DIV64_gv << ADC_PRESC_gp)   /* System clock divided by 64 */
} ADC_PRESC_t;

/* Reference select bit group values */
typedef enum ADC_REFSEL_VALUES_enum
{
    ADC_REFSEL_VDD_gv              = 0x00,  /* VDD */
    ADC_REFSEL_VREFA_gv            = 0x02,  /* External Reference */
    ADC_REFSEL_1024MV_gv           = 0x04,  /* Internal 1.024V Reference */
    ADC_REFSEL_2048MV_gv           = 0x05,  /* Internal 2.048V Reference */
    ADC_REFSEL_2500MV_gv           = 0x06,  /* Internal 2.5V Reference */
    ADC_REFSEL_4096MV_gv           = 0x07   /* Internal 4.096V Reference */
} ADC_REFSEL_values_t;

/* Reference select bit group configurations*/
typedef enum ADC_REFSEL_enum
{
    ADC_REFSEL_VDD_gc              = (ADC_REFSEL_VDD_gv << ADC_REFSEL_gp),  /* VDD */
    ADC_REFSEL_VREFA_gc            = (ADC_REFSEL_VREFA_gv << ADC_REFSEL_gp),  /* External Reference */
    ADC_REFSEL_1024MV_gc           = (ADC_REFSEL_1024MV_gv << ADC_REFSEL_gp),  /* Internal 1.024V Reference */
    ADC_REFSEL_2048MV_gc           = (ADC_REFSEL_2048MV_gv << ADC_REFSEL_gp),  /* Internal 2.048V Reference */
    ADC_REFSEL_2500MV_gc           = (ADC_REFSEL_2500MV_gv << ADC_REFSEL_gp),  /* Internal 2.5V Reference */
    ADC_REFSEL_4096MV_gc           = (ADC_REFSEL_4096MV_gv << ADC_REFSEL_gp)   /* Internal 4.096V Reference */
} ADC_REFSEL_t;

/* Sample numbers select bit group values */
typedef enum ADC_SAMPNUM_VALUES_enum
{
    ADC_SAMPNUM_NONE_gv            = 0x00,  /* No accumulation */
    ADC_SAMPNUM_ACC2_gv            = 0x01,  /* 2 samples accumulation */
    ADC_SAMPNUM_ACC4_gv            = 0x02,  /* 4 samples accumulation */
    ADC_SAMPNUM_ACC8_gv            = 0x03,  /* 8 samples accumulation */
    ADC_SAMPNUM_ACC16_gv           = 0x04,  /* 16 samples accumulation */
    ADC_SAMPNUM_ACC32_gv           = 0x05,  /* 32 samples accumulation */
    ADC_SAMPNUM_ACC64_gv           = 0x06,  /* 64 samples accumulation */
    ADC_SAMPNUM_ACC128_gv          = 0x07,  /* 128 samples accumulation */
    ADC_SAMPNUM_ACC256_gv          = 0x08,  /* 256 samples accumulation */
    ADC_SAMPNUM_ACC512_gv          = 0x09,  /* 512 samples accumulation */
    ADC_SAMPNUM_ACC1024_gv         = 0x0A   /* 1024 samples accumulation */
} ADC_SAMPNUM_values_t;

/* Sample numbers select bit group configurations*/
typedef enum ADC_SAMPNUM_enum
{
    ADC_SAMPNUM_NONE_gc            = (ADC_SAMPNUM_NONE_gv << ADC_SAMPNUM_gp),  /* No accumulation */
    ADC_SAMPNUM_ACC2_gc            = (ADC_SAMPNUM_ACC2_gv << ADC_SAMPNUM_gp),  /* 2 samples accumulation */
    ADC_SAMPNUM_ACC4_gc            = (ADC_SAMPNUM_ACC4_gv << ADC_SAMPNUM_gp),  /* 4 samples accumulation */
    ADC_SAMPNUM_ACC8_gc            = (ADC_SAMPNUM_ACC8_gv << ADC_SAMPNUM_gp),  /* 8 samples accumulation */
    ADC_SAMPNUM_ACC16_gc           = (ADC_SAMPNUM_ACC16_gv << ADC_SAMPNUM_gp),  /* 16 samples accumulation */
    ADC_SAMPNUM_ACC32_gc           = (ADC_SAMPNUM_ACC32_gv << ADC_SAMPNUM_gp),  /* 32 samples accumulation */
    ADC_SAMPNUM_ACC64_gc           = (ADC_SAMPNUM_ACC64_gv << ADC_SAMPNUM_gp),  /* 64 samples accumulation */
    ADC_SAMPNUM_ACC128_gc          = (ADC_SAMPNUM_ACC128_gv << ADC_SAMPNUM_gp),  /* 128 samples accumulation */
    ADC_SAMPNUM_ACC256_gc          = (ADC_SAMPNUM_ACC256_gv << ADC_SAMPNUM_gp),  /* 256 samples accumulation */
    ADC_SAMPNUM_ACC512_gc          = (ADC_SAMPNUM_ACC512_gv << ADC_SAMPNUM_gp),  /* 512 samples accumulation */
    ADC_SAMPNUM_ACC1024_gc         = (ADC_SAMPNUM_ACC1024_gv << ADC_SAMPNUM_gp)   /* 1024 samples accumulation */
} ADC_SAMPNUM_t;

/* Start command select bit group values */
typedef enum ADC_START_VALUES_enum
{
    ADC_START_STOP_gv              = 0x00,  /* Stop an ongoing conversion */
    ADC_START_IMMEDIATE_gv         = 0x01,  /* Start immediately */
    ADC_START_MUXPOS_WRITE_gv      = 0x02,  /* Start on MUXPOS write */
    ADC_START_MUXNEG_WRITE_gv      = 0x03,  /* Start on MUXNEG write */
    ADC_START_EVENT_TRIGGER_gv     = 0x04   /* Start on event */
} ADC_START_values_t;

/* Start command select bit group configurations*/
typedef enum ADC_START_enum
{
    ADC_START_STOP_gc              = (ADC_START_STOP_gv << ADC_START_gp),  /* Stop an ongoing conversion */
    ADC_START_IMMEDIATE_gc         = (ADC_START_IMMEDIATE_gv << ADC_START_gp),  /* Start immediately */
    ADC_START_MUXPOS_WRITE_gc      = (ADC_START_MUXPOS_WRITE_gv << ADC_START_gp),  /* Start on MUXPOS write */
    ADC_START_MUXNEG_WRITE_gc      = (ADC_START_MUXNEG_WRITE_gv << ADC_START_gp),  /* Start on MUXNEG write */
    ADC_START_EVENT_TRIGGER_gc     = (ADC_START_EVENT_TRIGGER_gv << ADC_START_gp)   /* Start on event */
} ADC_START_t;

/* VIA select bit group values */
typedef enum ADC_VIA_VALUES_enum
{
    ADC_VIA_ADC_gv                 = 0x00,  /* Via ADC */
    ADC_VIA_PGA_gv                 = 0x01   /* Via PGA */
} ADC_VIA_values_t;

/* VIA select bit group configurations*/
typedef enum ADC_VIA_enum
{
    ADC_VIA_ADC_gc                 = (ADC_VIA_ADC_gv << ADC_VIA_gp),  /* Via ADC */
    ADC_VIA_PGA_gc                 = (ADC_VIA_PGA_gv << ADC_VIA_gp)   /* Via PGA */
} ADC_VIA_t;

/* Window Comparator Mode select bit group values */
typedef enum ADC_WINCM_VALUES_enum
{
    ADC_WINCM_NONE_gv              = 0x00,  /* No Window Comparison */
    ADC_WINCM_BELOW_gv             = 0x01,  /* Below Window */
    ADC_WINCM_ABOVE_gv             = 0x02,  /* Above Window */
    ADC_WINCM_INSIDE_gv            = 0x03,  /* Inside Window */
    ADC_WINCM_OUTSIDE_gv           = 0x04   /* Outside Window */
} ADC_WINCM_values_t;

/* Window Comparator Mode select bit group configurations*/
typedef enum ADC_WINCM_enum
{
    ADC_WINCM_NONE_gc              = (ADC_WINCM_NONE_gv << ADC_WINCM_gp),  /* No Window Comparison */
    ADC_WINCM_BELOW_gc             = (ADC_WINCM_BELOW_gv << ADC_WINCM_gp),  /* Below Window */
    ADC_WINCM_ABOVE_gc             = (ADC_WINCM_ABOVE_gv << ADC_WINCM_gp),  /* Above Window */
    ADC_WINCM_INSIDE_gc            = (ADC_WINCM_INSIDE_gv << ADC_WINCM_gp),  /* Inside Window */
    ADC_WINCM_OUTSIDE_gc           = (ADC_WINCM_OUTSIDE_gv << ADC_WINCM_gp)   /* Outside Window */
} ADC_WINCM_t;

/* Window Mode Source select bit group values */
typedef enum ADC_WINSRC_VALUES_enum
{
    ADC_WINSRC_RESULT_gv           = 0x00,  /* Result register */
    ADC_WINSRC_SAMPLE_gv           = 0x01   /* Sample register */
} ADC_WINSRC_values_t;

/* Window Mode Source select bit group configurations*/
typedef enum ADC_WINSRC_enum
{
    ADC_WINSRC_RESULT_gc           = (ADC_WINSRC_RESULT_gv << ADC_WINSRC_bp),  /* Result register */
    ADC_WINSRC_SAMPLE_gc           = (ADC_WINSRC_SAMPLE_gv << ADC_WINSRC_bp)   /* Sample register */
} ADC_WINSRC_t;

/* End of extract */

#endif //_XT_IO_ADC2_H_
