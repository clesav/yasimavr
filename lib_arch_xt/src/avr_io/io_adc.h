/*
 * io_adc.h
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

#ifndef _XT_IO_ADC_H_
#define _XT_IO_ADC_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the ADC peripheral for the following device models:
 *  - atmega 0-series
 *  - attiny 0-series
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


/* ADC - Analog to Digital Converter */
/* ADC.CTRLA  bit masks and bit positions */
#define ADC_ENABLE_bm  0x01  /* ADC Enable bit mask. */
#define ADC_ENABLE_bp  0  /* ADC Enable bit position. */
#define ADC_FREERUN_bm  0x02  /* ADC Freerun mode bit mask. */
#define ADC_FREERUN_bp  1  /* ADC Freerun mode bit position. */
#define ADC_RESSEL_bm  0x04  /* ADC Resolution bit mask. */
#define ADC_RESSEL_bp  2  /* ADC Resolution bit position. */
#define ADC_RUNSTBY_bm  0x80  /* Run standby mode bit mask. */
#define ADC_RUNSTBY_bp  7  /* Run standby mode bit position. */

/* ADC.CTRLB  bit masks and bit positions */
#define ADC_SAMPNUM_gm  0x07  /* Accumulation Samples group mask. */
#define ADC_SAMPNUM_gp  0  /* Accumulation Samples group position. */
#define ADC_SAMPNUM_0_bm  (1<<0)  /* Accumulation Samples bit 0 mask. */
#define ADC_SAMPNUM_0_bp  0  /* Accumulation Samples bit 0 position. */
#define ADC_SAMPNUM_1_bm  (1<<1)  /* Accumulation Samples bit 1 mask. */
#define ADC_SAMPNUM_1_bp  1  /* Accumulation Samples bit 1 position. */
#define ADC_SAMPNUM_2_bm  (1<<2)  /* Accumulation Samples bit 2 mask. */
#define ADC_SAMPNUM_2_bp  2  /* Accumulation Samples bit 2 position. */

/* ADC.CTRLC  bit masks and bit positions */
#define ADC_PRESC_gm  0x07  /* Clock Pre-scaler group mask. */
#define ADC_PRESC_gp  0  /* Clock Pre-scaler group position. */
#define ADC_PRESC_0_bm  (1<<0)  /* Clock Pre-scaler bit 0 mask. */
#define ADC_PRESC_0_bp  0  /* Clock Pre-scaler bit 0 position. */
#define ADC_PRESC_1_bm  (1<<1)  /* Clock Pre-scaler bit 1 mask. */
#define ADC_PRESC_1_bp  1  /* Clock Pre-scaler bit 1 position. */
#define ADC_PRESC_2_bm  (1<<2)  /* Clock Pre-scaler bit 2 mask. */
#define ADC_PRESC_2_bp  2  /* Clock Pre-scaler bit 2 position. */
#define ADC_REFSEL_gm  0x30  /* Reference Selection group mask. */
#define ADC_REFSEL_gp  4  /* Reference Selection group position. */
#define ADC_REFSEL_0_bm  (1<<4)  /* Reference Selection bit 0 mask. */
#define ADC_REFSEL_0_bp  4  /* Reference Selection bit 0 position. */
#define ADC_REFSEL_1_bm  (1<<5)  /* Reference Selection bit 1 mask. */
#define ADC_REFSEL_1_bp  5  /* Reference Selection bit 1 position. */
#define ADC_SAMPCAP_bm  0x40  /* Sample Capacitance Selection bit mask. */
#define ADC_SAMPCAP_bp  6  /* Sample Capacitance Selection bit position. */

/* ADC.CTRLD  bit masks and bit positions */
#define ADC_SAMPDLY_gm  0x0F  /* Sampling Delay Selection group mask. */
#define ADC_SAMPDLY_gp  0  /* Sampling Delay Selection group position. */
#define ADC_SAMPDLY_0_bm  (1<<0)  /* Sampling Delay Selection bit 0 mask. */
#define ADC_SAMPDLY_0_bp  0  /* Sampling Delay Selection bit 0 position. */
#define ADC_SAMPDLY_1_bm  (1<<1)  /* Sampling Delay Selection bit 1 mask. */
#define ADC_SAMPDLY_1_bp  1  /* Sampling Delay Selection bit 1 position. */
#define ADC_SAMPDLY_2_bm  (1<<2)  /* Sampling Delay Selection bit 2 mask. */
#define ADC_SAMPDLY_2_bp  2  /* Sampling Delay Selection bit 2 position. */
#define ADC_SAMPDLY_3_bm  (1<<3)  /* Sampling Delay Selection bit 3 mask. */
#define ADC_SAMPDLY_3_bp  3  /* Sampling Delay Selection bit 3 position. */
#define ADC_ASDV_bm  0x10  /* Automatic Sampling Delay Variation bit mask. */
#define ADC_ASDV_bp  4  /* Automatic Sampling Delay Variation bit position. */
#define ADC_INITDLY_gm  0xE0  /* Initial Delay Selection group mask. */
#define ADC_INITDLY_gp  5  /* Initial Delay Selection group position. */
#define ADC_INITDLY_0_bm  (1<<5)  /* Initial Delay Selection bit 0 mask. */
#define ADC_INITDLY_0_bp  5  /* Initial Delay Selection bit 0 position. */
#define ADC_INITDLY_1_bm  (1<<6)  /* Initial Delay Selection bit 1 mask. */
#define ADC_INITDLY_1_bp  6  /* Initial Delay Selection bit 1 position. */
#define ADC_INITDLY_2_bm  (1<<7)  /* Initial Delay Selection bit 2 mask. */
#define ADC_INITDLY_2_bp  7  /* Initial Delay Selection bit 2 position. */

/* ADC.CTRLE  bit masks and bit positions */
#define ADC_WINCM_gm  0x07  /* Window Comparator Mode group mask. */
#define ADC_WINCM_gp  0  /* Window Comparator Mode group position. */
#define ADC_WINCM_0_bm  (1<<0)  /* Window Comparator Mode bit 0 mask. */
#define ADC_WINCM_0_bp  0  /* Window Comparator Mode bit 0 position. */
#define ADC_WINCM_1_bm  (1<<1)  /* Window Comparator Mode bit 1 mask. */
#define ADC_WINCM_1_bp  1  /* Window Comparator Mode bit 1 position. */
#define ADC_WINCM_2_bm  (1<<2)  /* Window Comparator Mode bit 2 mask. */
#define ADC_WINCM_2_bp  2  /* Window Comparator Mode bit 2 position. */

/* ADC.SAMPCTRL  bit masks and bit positions */
#define ADC_SAMPLEN_gm  0x1F  /* Sample length group mask. */
#define ADC_SAMPLEN_gp  0  /* Sample length group position. */
#define ADC_SAMPLEN_0_bm  (1<<0)  /* Sample length bit 0 mask. */
#define ADC_SAMPLEN_0_bp  0  /* Sample length bit 0 position. */
#define ADC_SAMPLEN_1_bm  (1<<1)  /* Sample length bit 1 mask. */
#define ADC_SAMPLEN_1_bp  1  /* Sample length bit 1 position. */
#define ADC_SAMPLEN_2_bm  (1<<2)  /* Sample length bit 2 mask. */
#define ADC_SAMPLEN_2_bp  2  /* Sample length bit 2 position. */
#define ADC_SAMPLEN_3_bm  (1<<3)  /* Sample length bit 3 mask. */
#define ADC_SAMPLEN_3_bp  3  /* Sample length bit 3 position. */
#define ADC_SAMPLEN_4_bm  (1<<4)  /* Sample length bit 4 mask. */
#define ADC_SAMPLEN_4_bp  4  /* Sample length bit 4 position. */

/* ADC.MUXPOS  bit masks and bit positions */
#define ADC_MUXPOS_gm  0x1F  /* Analog Channel Selection Bits group mask. */
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

/* ADC.COMMAND  bit masks and bit positions */
#define ADC_STCONV_bm  0x01  /* Start Conversion Operation bit mask. */
#define ADC_STCONV_bp  0  /* Start Conversion Operation bit position. */

/* ADC.EVCTRL  bit masks and bit positions */
#define ADC_STARTEI_bm  0x01  /* Start Event Input Enable bit mask. */
#define ADC_STARTEI_bp  0  /* Start Event Input Enable bit position. */

/* ADC.INTCTRL  bit masks and bit positions */
#define ADC_RESRDY_bm  0x01  /* Result Ready Interrupt Enable bit mask. */
#define ADC_RESRDY_bp  0  /* Result Ready Interrupt Enable bit position. */
#define ADC_WCMP_bm  0x02  /* Window Comparator Interrupt Enable bit mask. */
#define ADC_WCMP_bp  1  /* Window Comparator Interrupt Enable bit position. */

/* ADC.INTFLAGS  bit masks and bit positions */
/* ADC_RESRDY  is already defined. */
/* ADC_WCMP  is already defined. */

/* ADC.DBGCTRL  bit masks and bit positions */
#define ADC_DBGRUN_bm  0x01  /* Debug run bit mask. */
#define ADC_DBGRUN_bp  0  /* Debug run bit position. */

/* ADC.TEMP  bit masks and bit positions */
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

/* ADC.CALIB  bit masks and bit positions */
#define ADC_DUTYCYC_bm  0x01  /* Duty Cycle bit mask. */
#define ADC_DUTYCYC_bp  0  /* Duty Cycle bit position. */


/* Analog to Digital Converter */
typedef struct ADC_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t CTRLC;  /* Control C */
    register8_t CTRLD;  /* Control D */
    register8_t CTRLE;  /* Control E */
    register8_t SAMPCTRL;  /* Sample Control */
    register8_t MUXPOS;  /* Positive mux input */
    register8_t reserved_1[1];
    register8_t COMMAND;  /* Command */
    register8_t EVCTRL;  /* Event Control */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t DBGCTRL;  /* Debug Control */
    register8_t TEMP;  /* Temporary Data */
    register8_t reserved_2[2];
    _WORDREGISTER(RES);  /* ADC Accumulator Result */
    _WORDREGISTER(WINLT);  /* Window comparator low threshold */
    _WORDREGISTER(WINHT);  /* Window comparator high threshold */
    register8_t CALIB;  /* Calibration */
    register8_t reserved_3[1];
} ADC_t;

/* Automatic Sampling Delay Variation select bit group values */
typedef enum ADC_ASDV_VALUES_enum
{
    ADC_ASDV_ASVOFF_gv             = 0x00,  /* The Automatic Sampling Delay Variation is disabled */
    ADC_ASDV_ASVON_gv              = 0x01   /* The Automatic Sampling Delay Variation is enabled */
} ADC_ASDV_values_t;

/* Automatic Sampling Delay Variation select bit group configurations*/
typedef enum ADC_ASDV_enum
{
    ADC_ASDV_ASVOFF_gc             = (ADC_ASDV_ASVOFF_gv << ADC_ASDV_bp),  /* The Automatic Sampling Delay Variation is disabled */
    ADC_ASDV_ASVON_gc              = (ADC_ASDV_ASVON_gv << ADC_ASDV_bp)   /* The Automatic Sampling Delay Variation is enabled */
} ADC_ASDV_t;

/* Duty Cycle select bit group values */
typedef enum ADC_DUTYCYC_VALUES_enum
{
    ADC_DUTYCYC_DUTY50_gv          = 0x00,  /* 50% Duty cycle */
    ADC_DUTYCYC_DUTY25_gv          = 0x01   /* 25% Duty cycle */
} ADC_DUTYCYC_values_t;

/* Duty Cycle select bit group configurations*/
typedef enum ADC_DUTYCYC_enum
{
    ADC_DUTYCYC_DUTY50_gc          = (ADC_DUTYCYC_DUTY50_gv << ADC_DUTYCYC_bp),  /* 50% Duty cycle */
    ADC_DUTYCYC_DUTY25_gc          = (ADC_DUTYCYC_DUTY25_gv << ADC_DUTYCYC_bp)   /* 25% Duty cycle */
} ADC_DUTYCYC_t;

/* Initial Delay Selection bit group values */
typedef enum ADC_INITDLY_VALUES_enum
{
    ADC_INITDLY_DLY0_gv            = 0x00,  /* Delay 0 CLK_ADC cycles */
    ADC_INITDLY_DLY16_gv           = 0x01,  /* Delay 16 CLK_ADC cycles */
    ADC_INITDLY_DLY32_gv           = 0x02,  /* Delay 32 CLK_ADC cycles */
    ADC_INITDLY_DLY64_gv           = 0x03,  /* Delay 64 CLK_ADC cycles */
    ADC_INITDLY_DLY128_gv          = 0x04,  /* Delay 128 CLK_ADC cycles */
    ADC_INITDLY_DLY256_gv          = 0x05   /* Delay 256 CLK_ADC cycles */
} ADC_INITDLY_values_t;

/* Initial Delay Selection bit group configurations*/
typedef enum ADC_INITDLY_enum
{
    ADC_INITDLY_DLY0_gc            = (ADC_INITDLY_DLY0_gv << ADC_INITDLY_gp),  /* Delay 0 CLK_ADC cycles */
    ADC_INITDLY_DLY16_gc           = (ADC_INITDLY_DLY16_gv << ADC_INITDLY_gp),  /* Delay 16 CLK_ADC cycles */
    ADC_INITDLY_DLY32_gc           = (ADC_INITDLY_DLY32_gv << ADC_INITDLY_gp),  /* Delay 32 CLK_ADC cycles */
    ADC_INITDLY_DLY64_gc           = (ADC_INITDLY_DLY64_gv << ADC_INITDLY_gp),  /* Delay 64 CLK_ADC cycles */
    ADC_INITDLY_DLY128_gc          = (ADC_INITDLY_DLY128_gv << ADC_INITDLY_gp),  /* Delay 128 CLK_ADC cycles */
    ADC_INITDLY_DLY256_gc          = (ADC_INITDLY_DLY256_gv << ADC_INITDLY_gp)   /* Delay 256 CLK_ADC cycles */
} ADC_INITDLY_t;

/* Analog Channel Selection Bits bit group values */
typedef enum ADC_MUXPOS_VALUES_enum
{
    ADC_MUXPOS_AIN0_gv             = 0x00,  /* ADC input pin 0 */
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
    ADC_MUXPOS_DACREF_gv           = 0x1C,  /* AC DAC Reference */
    ADC_MUXPOS_TEMPSENSE_gv        = 0x1E,  /* Temperature sensor */
    ADC_MUXPOS_GND_gv              = 0x1F   /* 0V (GND) */
} ADC_MUXPOS_values_t;

/* Analog Channel Selection Bits bit group configurations*/
typedef enum ADC_MUXPOS_enum
{
    ADC_MUXPOS_AIN0_gc             = (ADC_MUXPOS_AIN0_gv << ADC_MUXPOS_gp),  /* ADC input pin 0 */
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
    ADC_MUXPOS_DACREF_gc           = (ADC_MUXPOS_DACREF_gv << ADC_MUXPOS_gp),  /* AC DAC Reference */
    ADC_MUXPOS_TEMPSENSE_gc        = (ADC_MUXPOS_TEMPSENSE_gv << ADC_MUXPOS_gp),  /* Temperature sensor */
    ADC_MUXPOS_GND_gc              = (ADC_MUXPOS_GND_gv << ADC_MUXPOS_gp)   /* 0V (GND) */
} ADC_MUXPOS_t;

/* Clock Pre-scaler select bit group values */
typedef enum ADC_PRESC_VALUES_enum
{
    ADC_PRESC_DIV2_gv              = 0x00,  /* CLK_PER divided by 2 */
    ADC_PRESC_DIV4_gv              = 0x01,  /* CLK_PER divided by 4 */
    ADC_PRESC_DIV8_gv              = 0x02,  /* CLK_PER divided by 8 */
    ADC_PRESC_DIV16_gv             = 0x03,  /* CLK_PER divided by 16 */
    ADC_PRESC_DIV32_gv             = 0x04,  /* CLK_PER divided by 32 */
    ADC_PRESC_DIV64_gv             = 0x05,  /* CLK_PER divided by 64 */
    ADC_PRESC_DIV128_gv            = 0x06,  /* CLK_PER divided by 128 */
    ADC_PRESC_DIV256_gv            = 0x07   /* CLK_PER divided by 256 */
} ADC_PRESC_values_t;

/* Clock Pre-scaler select bit group configurations*/
typedef enum ADC_PRESC_enum
{
    ADC_PRESC_DIV2_gc              = (ADC_PRESC_DIV2_gv << ADC_PRESC_gp),  /* CLK_PER divided by 2 */
    ADC_PRESC_DIV4_gc              = (ADC_PRESC_DIV4_gv << ADC_PRESC_gp),  /* CLK_PER divided by 4 */
    ADC_PRESC_DIV8_gc              = (ADC_PRESC_DIV8_gv << ADC_PRESC_gp),  /* CLK_PER divided by 8 */
    ADC_PRESC_DIV16_gc             = (ADC_PRESC_DIV16_gv << ADC_PRESC_gp),  /* CLK_PER divided by 16 */
    ADC_PRESC_DIV32_gc             = (ADC_PRESC_DIV32_gv << ADC_PRESC_gp),  /* CLK_PER divided by 32 */
    ADC_PRESC_DIV64_gc             = (ADC_PRESC_DIV64_gv << ADC_PRESC_gp),  /* CLK_PER divided by 64 */
    ADC_PRESC_DIV128_gc            = (ADC_PRESC_DIV128_gv << ADC_PRESC_gp),  /* CLK_PER divided by 128 */
    ADC_PRESC_DIV256_gc            = (ADC_PRESC_DIV256_gv << ADC_PRESC_gp)   /* CLK_PER divided by 256 */
} ADC_PRESC_t;

/* Reference Selection bit group values */
typedef enum ADC_REFSEL_VALUES_enum
{
    ADC_REFSEL_INTREF_gv           = 0x00,  /* Internal reference */
    ADC_REFSEL_VDDREF_gv           = 0x01,  /* VDD */
    ADC_REFSEL_VREFA_gv            = 0x02   /* External reference */
} ADC_REFSEL_values_t;

/* Reference Selection bit group configurations*/
typedef enum ADC_REFSEL_enum
{
    ADC_REFSEL_INTREF_gc           = (ADC_REFSEL_INTREF_gv << ADC_REFSEL_gp),  /* Internal reference */
    ADC_REFSEL_VDDREF_gc           = (ADC_REFSEL_VDDREF_gv << ADC_REFSEL_gp),  /* VDD */
    ADC_REFSEL_VREFA_gc            = (ADC_REFSEL_VREFA_gv << ADC_REFSEL_gp)   /* External reference */
} ADC_REFSEL_t;

/* ADC Resolution select bit group values */
typedef enum ADC_RESSEL_VALUES_enum
{
    ADC_RESSEL_10BIT_gv            = 0x00,  /* 10-bit mode */
    ADC_RESSEL_8BIT_gv             = 0x01   /* 8-bit mode */
} ADC_RESSEL_values_t;

/* ADC Resolution select bit group configurations*/
typedef enum ADC_RESSEL_enum
{
    ADC_RESSEL_10BIT_gc            = (ADC_RESSEL_10BIT_gv << ADC_RESSEL_bp),  /* 10-bit mode */
    ADC_RESSEL_8BIT_gc             = (ADC_RESSEL_8BIT_gv << ADC_RESSEL_bp)   /* 8-bit mode */
} ADC_RESSEL_t;

/* Accumulation Samples select bit group values */
typedef enum ADC_SAMPNUM_VALUES_enum
{
    ADC_SAMPNUM_ACC1_gv            = 0x00,  /* 1 ADC sample */
    ADC_SAMPNUM_ACC2_gv            = 0x01,  /* Accumulate 2 samples */
    ADC_SAMPNUM_ACC4_gv            = 0x02,  /* Accumulate 4 samples */
    ADC_SAMPNUM_ACC8_gv            = 0x03,  /* Accumulate 8 samples */
    ADC_SAMPNUM_ACC16_gv           = 0x04,  /* Accumulate 16 samples */
    ADC_SAMPNUM_ACC32_gv           = 0x05,  /* Accumulate 32 samples */
    ADC_SAMPNUM_ACC64_gv           = 0x06   /* Accumulate 64 samples */
} ADC_SAMPNUM_values_t;

/* Accumulation Samples select bit group configurations*/
typedef enum ADC_SAMPNUM_enum
{
    ADC_SAMPNUM_ACC1_gc            = (ADC_SAMPNUM_ACC1_gv << ADC_SAMPNUM_gp),  /* 1 ADC sample */
    ADC_SAMPNUM_ACC2_gc            = (ADC_SAMPNUM_ACC2_gv << ADC_SAMPNUM_gp),  /* Accumulate 2 samples */
    ADC_SAMPNUM_ACC4_gc            = (ADC_SAMPNUM_ACC4_gv << ADC_SAMPNUM_gp),  /* Accumulate 4 samples */
    ADC_SAMPNUM_ACC8_gc            = (ADC_SAMPNUM_ACC8_gv << ADC_SAMPNUM_gp),  /* Accumulate 8 samples */
    ADC_SAMPNUM_ACC16_gc           = (ADC_SAMPNUM_ACC16_gv << ADC_SAMPNUM_gp),  /* Accumulate 16 samples */
    ADC_SAMPNUM_ACC32_gc           = (ADC_SAMPNUM_ACC32_gv << ADC_SAMPNUM_gp),  /* Accumulate 32 samples */
    ADC_SAMPNUM_ACC64_gc           = (ADC_SAMPNUM_ACC64_gv << ADC_SAMPNUM_gp)   /* Accumulate 64 samples */
} ADC_SAMPNUM_t;

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

/* End of extract */

#endif //_XT_IO_ADC_H_
