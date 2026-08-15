/*
 * io_nvm.h
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

#ifndef _XT_IO_NVM_H_
#define _XT_IO_NVM_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the NVMCTRL/FUSE peripherals for the following device models:
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


/* FUSE - Fuses */
/* FUSE.WDTCFG  bit masks and bit positions */
#define FUSE_PERIOD_gm  0x0F  /* Watchdog Timeout Period group mask. */
#define FUSE_PERIOD_gp  0  /* Watchdog Timeout Period group position. */
#define FUSE_PERIOD_0_bm  (1<<0)  /* Watchdog Timeout Period bit 0 mask. */
#define FUSE_PERIOD_0_bp  0  /* Watchdog Timeout Period bit 0 position. */
#define FUSE_PERIOD_1_bm  (1<<1)  /* Watchdog Timeout Period bit 1 mask. */
#define FUSE_PERIOD_1_bp  1  /* Watchdog Timeout Period bit 1 position. */
#define FUSE_PERIOD_2_bm  (1<<2)  /* Watchdog Timeout Period bit 2 mask. */
#define FUSE_PERIOD_2_bp  2  /* Watchdog Timeout Period bit 2 position. */
#define FUSE_PERIOD_3_bm  (1<<3)  /* Watchdog Timeout Period bit 3 mask. */
#define FUSE_PERIOD_3_bp  3  /* Watchdog Timeout Period bit 3 position. */
#define FUSE_WINDOW_gm  0xF0  /* Watchdog Window Timeout Period group mask. */
#define FUSE_WINDOW_gp  4  /* Watchdog Window Timeout Period group position. */
#define FUSE_WINDOW_0_bm  (1<<4)  /* Watchdog Window Timeout Period bit 0 mask. */
#define FUSE_WINDOW_0_bp  4  /* Watchdog Window Timeout Period bit 0 position. */
#define FUSE_WINDOW_1_bm  (1<<5)  /* Watchdog Window Timeout Period bit 1 mask. */
#define FUSE_WINDOW_1_bp  5  /* Watchdog Window Timeout Period bit 1 position. */
#define FUSE_WINDOW_2_bm  (1<<6)  /* Watchdog Window Timeout Period bit 2 mask. */
#define FUSE_WINDOW_2_bp  6  /* Watchdog Window Timeout Period bit 2 position. */
#define FUSE_WINDOW_3_bm  (1<<7)  /* Watchdog Window Timeout Period bit 3 mask. */
#define FUSE_WINDOW_3_bp  7  /* Watchdog Window Timeout Period bit 3 position. */

/* FUSE.BODCFG  bit masks and bit positions */
#define FUSE_SLEEP_gm  0x03  /* BOD Operation in Sleep Mode group mask. */
#define FUSE_SLEEP_gp  0  /* BOD Operation in Sleep Mode group position. */
#define FUSE_SLEEP_0_bm  (1<<0)  /* BOD Operation in Sleep Mode bit 0 mask. */
#define FUSE_SLEEP_0_bp  0  /* BOD Operation in Sleep Mode bit 0 position. */
#define FUSE_SLEEP_1_bm  (1<<1)  /* BOD Operation in Sleep Mode bit 1 mask. */
#define FUSE_SLEEP_1_bp  1  /* BOD Operation in Sleep Mode bit 1 position. */
#define FUSE_ACTIVE_gm  0x0C  /* BOD Operation in Active Mode group mask. */
#define FUSE_ACTIVE_gp  2  /* BOD Operation in Active Mode group position. */
#define FUSE_ACTIVE_0_bm  (1<<2)  /* BOD Operation in Active Mode bit 0 mask. */
#define FUSE_ACTIVE_0_bp  2  /* BOD Operation in Active Mode bit 0 position. */
#define FUSE_ACTIVE_1_bm  (1<<3)  /* BOD Operation in Active Mode bit 1 mask. */
#define FUSE_ACTIVE_1_bp  3  /* BOD Operation in Active Mode bit 1 position. */
#define FUSE_SAMPFREQ_bm  0x10  /* BOD Sample Frequency bit mask. */
#define FUSE_SAMPFREQ_bp  4  /* BOD Sample Frequency bit position. */
#define FUSE_LVL_gm  0xE0  /* BOD Level group mask. */
#define FUSE_LVL_gp  5  /* BOD Level group position. */
#define FUSE_LVL_0_bm  (1<<5)  /* BOD Level bit 0 mask. */
#define FUSE_LVL_0_bp  5  /* BOD Level bit 0 position. */
#define FUSE_LVL_1_bm  (1<<6)  /* BOD Level bit 1 mask. */
#define FUSE_LVL_1_bp  6  /* BOD Level bit 1 position. */
#define FUSE_LVL_2_bm  (1<<7)  /* BOD Level bit 2 mask. */
#define FUSE_LVL_2_bp  7  /* BOD Level bit 2 position. */

/* FUSE.OSCCFG  bit masks and bit positions */
#define FUSE_FREQSEL_gm  0x03  /* Frequency Select group mask. */
#define FUSE_FREQSEL_gp  0  /* Frequency Select group position. */
#define FUSE_FREQSEL_0_bm  (1<<0)  /* Frequency Select bit 0 mask. */
#define FUSE_FREQSEL_0_bp  0  /* Frequency Select bit 0 position. */
#define FUSE_FREQSEL_1_bm  (1<<1)  /* Frequency Select bit 1 mask. */
#define FUSE_FREQSEL_1_bp  1  /* Frequency Select bit 1 position. */
#define FUSE_OSCLOCK_bm  0x80  /* Oscillator Lock bit mask. */
#define FUSE_OSCLOCK_bp  7  /* Oscillator Lock bit position. */

/* FUSE.TCD0CFG  bit masks and bit positions */
#define FUSE_CMPA_bm  0x01  /* Compare A Default Output Value bit mask. */
#define FUSE_CMPA_bp  0  /* Compare A Default Output Value bit position. */
#define FUSE_CMPB_bm  0x02  /* Compare B Default Output Value bit mask. */
#define FUSE_CMPB_bp  1  /* Compare B Default Output Value bit position. */
#define FUSE_CMPC_bm  0x04  /* Compare C Default Output Value bit mask. */
#define FUSE_CMPC_bp  2  /* Compare C Default Output Value bit position. */
#define FUSE_CMPD_bm  0x08  /* Compare D Default Output Value bit mask. */
#define FUSE_CMPD_bp  3  /* Compare D Default Output Value bit position. */
#define FUSE_CMPAEN_bm  0x10  /* Compare A Output Enable bit mask. */
#define FUSE_CMPAEN_bp  4  /* Compare A Output Enable bit position. */
#define FUSE_CMPBEN_bm  0x20  /* Compare B Output Enable bit mask. */
#define FUSE_CMPBEN_bp  5  /* Compare B Output Enable bit position. */
#define FUSE_CMPCEN_bm  0x40  /* Compare C Output Enable bit mask. */
#define FUSE_CMPCEN_bp  6  /* Compare C Output Enable bit position. */
#define FUSE_CMPDEN_bm  0x80  /* Compare D Output Enable bit mask. */
#define FUSE_CMPDEN_bp  7  /* Compare D Output Enable bit position. */

/* FUSE.SYSCFG0  bit masks and bit positions */
#define FUSE_EESAVE_bm  0x01  /* EEPROM Save bit mask. */
#define FUSE_EESAVE_bp  0  /* EEPROM Save bit position. */
#define FUSE_RSTPINCFG_bm  0x08  /* Reset Pin Configuration bit mask. */
#define FUSE_RSTPINCFG_bp  3  /* Reset Pin Configuration bit position. */
#define FUSE_CRCSRC_gm  0xC0  /* CRC Source group mask. */
#define FUSE_CRCSRC_gp  6  /* CRC Source group position. */
#define FUSE_CRCSRC_0_bm  (1<<6)  /* CRC Source bit 0 mask. */
#define FUSE_CRCSRC_0_bp  6  /* CRC Source bit 0 position. */
#define FUSE_CRCSRC_1_bm  (1<<7)  /* CRC Source bit 1 mask. */
#define FUSE_CRCSRC_1_bp  7  /* CRC Source bit 1 position. */

/* FUSE.SYSCFG1  bit masks and bit positions */
#define FUSE_SUT_gm  0x07  /* Startup Time group mask. */
#define FUSE_SUT_gp  0  /* Startup Time group position. */
#define FUSE_SUT_0_bm  (1<<0)  /* Startup Time bit 0 mask. */
#define FUSE_SUT_0_bp  0  /* Startup Time bit 0 position. */
#define FUSE_SUT_1_bm  (1<<1)  /* Startup Time bit 1 mask. */
#define FUSE_SUT_1_bp  1  /* Startup Time bit 1 position. */
#define FUSE_SUT_2_bm  (1<<2)  /* Startup Time bit 2 mask. */
#define FUSE_SUT_2_bp  2  /* Startup Time bit 2 position. */


/* NVMCTRL - Non-volatile Memory Controller */
/* NVMCTRL.CTRLA  bit masks and bit positions */
#define NVMCTRL_CMD_gm  0x07  /* Command group mask. */
#define NVMCTRL_CMD_gp  0  /* Command group position. */
#define NVMCTRL_CMD_0_bm  (1<<0)  /* Command bit 0 mask. */
#define NVMCTRL_CMD_0_bp  0  /* Command bit 0 position. */
#define NVMCTRL_CMD_1_bm  (1<<1)  /* Command bit 1 mask. */
#define NVMCTRL_CMD_1_bp  1  /* Command bit 1 position. */
#define NVMCTRL_CMD_2_bm  (1<<2)  /* Command bit 2 mask. */
#define NVMCTRL_CMD_2_bp  2  /* Command bit 2 position. */

/* NVMCTRL.CTRLB  bit masks and bit positions */
#define NVMCTRL_APCWP_bm  0x01  /* Application code write protect bit mask. */
#define NVMCTRL_APCWP_bp  0  /* Application code write protect bit position. */
#define NVMCTRL_BOOTLOCK_bm  0x02  /* Boot Lock bit mask. */
#define NVMCTRL_BOOTLOCK_bp  1  /* Boot Lock bit position. */

/* NVMCTRL.STATUS  bit masks and bit positions */
#define NVMCTRL_FBUSY_bm  0x01  /* Flash busy bit mask. */
#define NVMCTRL_FBUSY_bp  0  /* Flash busy bit position. */
#define NVMCTRL_EEBUSY_bm  0x02  /* EEPROM busy bit mask. */
#define NVMCTRL_EEBUSY_bp  1  /* EEPROM busy bit position. */
#define NVMCTRL_WRERROR_bm  0x04  /* Write error bit mask. */
#define NVMCTRL_WRERROR_bp  2  /* Write error bit position. */

/* NVMCTRL.INTCTRL  bit masks and bit positions */
#define NVMCTRL_EEREADY_bm  0x01  /* EEPROM Ready bit mask. */
#define NVMCTRL_EEREADY_bp  0  /* EEPROM Ready bit position. */

/* NVMCTRL.INTFLAGS  bit masks and bit positions */
/* NVMCTRL_EEREADY  is already defined. */


/* Fuses */
typedef struct FUSE_struct
{
    register8_t WDTCFG;  /* Watchdog Configuration */
    register8_t BODCFG;  /* BOD Configuration */
    register8_t OSCCFG;  /* Oscillator Configuration */
    register8_t reserved_1[1];
    register8_t TCD0CFG;  /* TCD0 Configuration */
    register8_t SYSCFG0;  /* System Configuration 0 */
    register8_t SYSCFG1;  /* System Configuration 1 */
    register8_t APPEND;  /* Application Code Section End */
    register8_t BOOTEND;  /* Boot Section End */
} FUSE_t;

/* BOD Operation in Active Mode select bit group values */
typedef enum ACTIVE_VALUES_enum
{
    ACTIVE_DIS_gv                  = 0x00,  /* Disabled */
    ACTIVE_ENABLED_gv              = 0x01,  /* Enabled */
    ACTIVE_SAMPLED_gv              = 0x02,  /* Sampled */
    ACTIVE_ENWAKE_gv               = 0x03   /* Enabled with wake-up halted until BOD is ready */
} ACTIVE_values_t;

/* BOD Operation in Active Mode select bit group configurations*/
typedef enum ACTIVE_enum
{
    ACTIVE_DIS_gc                  = (ACTIVE_DIS_gv << FUSE_ACTIVE_gp),  /* Disabled */
    ACTIVE_ENABLED_gc              = (ACTIVE_ENABLED_gv << FUSE_ACTIVE_gp),  /* Enabled */
    ACTIVE_SAMPLED_gc              = (ACTIVE_SAMPLED_gv << FUSE_ACTIVE_gp),  /* Sampled */
    ACTIVE_ENWAKE_gc               = (ACTIVE_ENWAKE_gv << FUSE_ACTIVE_gp)   /* Enabled with wake-up halted until BOD is ready */
} ACTIVE_t;

/* CRC Source select bit group values */
typedef enum CRCSRC_VALUES_enum
{
    CRCSRC_FLASH_gv                = 0x00,  /* The CRC is performed on the entire Flash (boot, application code and application data section). */
    CRCSRC_BOOT_gv                 = 0x01,  /* The CRC is performed on the boot section of Flash */
    CRCSRC_BOOTAPP_gv              = 0x02,  /* The CRC is performed on the boot and application code section of Flash */
    CRCSRC_NOCRC_gv                = 0x03   /* Disable CRC. */
} CRCSRC_values_t;

/* CRC Source select bit group configurations*/
typedef enum CRCSRC_enum
{
    CRCSRC_FLASH_gc                = (CRCSRC_FLASH_gv << FUSE_CRCSRC_gp),  /* The CRC is performed on the entire Flash (boot, application code and application data section). */
    CRCSRC_BOOT_gc                 = (CRCSRC_BOOT_gv << FUSE_CRCSRC_gp),  /* The CRC is performed on the boot section of Flash */
    CRCSRC_BOOTAPP_gc              = (CRCSRC_BOOTAPP_gv << FUSE_CRCSRC_gp),  /* The CRC is performed on the boot and application code section of Flash */
    CRCSRC_NOCRC_gc                = (CRCSRC_NOCRC_gv << FUSE_CRCSRC_gp)   /* Disable CRC. */
} CRCSRC_t;

/* Frequency Select bit group values */
typedef enum FREQSEL_VALUES_enum
{
    FREQSEL_16MHZ_gv               = 0x01,  /* 16 MHz */
    FREQSEL_20MHZ_gv               = 0x02   /* 20 MHz */
} FREQSEL_values_t;

/* Frequency Select bit group configurations*/
typedef enum FREQSEL_enum
{
    FREQSEL_16MHZ_gc               = (FREQSEL_16MHZ_gv << FUSE_FREQSEL_gp),  /* 16 MHz */
    FREQSEL_20MHZ_gc               = (FREQSEL_20MHZ_gv << FUSE_FREQSEL_gp)   /* 20 MHz */
} FREQSEL_t;

/* BOD Level select bit group values */
typedef enum LVL_VALUES_enum
{
    LVL_BODLEVEL0_gv               = 0x00,  /* 1.8 V */
    LVL_BODLEVEL2_gv               = 0x02,  /* 2.6 V */
    LVL_BODLEVEL7_gv               = 0x07   /* 4.2 V */
} LVL_values_t;

/* BOD Level select bit group configurations*/
typedef enum LVL_enum
{
    LVL_BODLEVEL0_gc               = (LVL_BODLEVEL0_gv << FUSE_LVL_gp),  /* 1.8 V */
    LVL_BODLEVEL2_gc               = (LVL_BODLEVEL2_gv << FUSE_LVL_gp),  /* 2.6 V */
    LVL_BODLEVEL7_gc               = (LVL_BODLEVEL7_gv << FUSE_LVL_gp)   /* 4.2 V */
} LVL_t;

/* Watchdog Timeout Period select bit group values */
typedef enum PERIOD_VALUES_enum
{
    PERIOD_OFF_gv                  = 0x00,  /* Off */
    PERIOD_8CLK_gv                 = 0x01,  /* 8 cycles (8ms) */
    PERIOD_16CLK_gv                = 0x02,  /* 16 cycles (16ms) */
    PERIOD_32CLK_gv                = 0x03,  /* 32 cycles (32ms) */
    PERIOD_64CLK_gv                = 0x04,  /* 64 cycles (64ms) */
    PERIOD_128CLK_gv               = 0x05,  /* 128 cycles (0.128s) */
    PERIOD_256CLK_gv               = 0x06,  /* 256 cycles (0.256s) */
    PERIOD_512CLK_gv               = 0x07,  /* 512 cycles (0.512s) */
    PERIOD_1KCLK_gv                = 0x08,  /* 1K cycles (1.0s) */
    PERIOD_2KCLK_gv                = 0x09,  /* 2K cycles (2.0s) */
    PERIOD_4KCLK_gv                = 0x0A,  /* 4K cycles (4.1s) */
    PERIOD_8KCLK_gv                = 0x0B   /* 8K cycles (8.2s) */
} PERIOD_values_t;

/* Watchdog Timeout Period select bit group configurations*/
typedef enum PERIOD_enum
{
    PERIOD_OFF_gc                  = (PERIOD_OFF_gv << FUSE_PERIOD_gp),  /* Off */
    PERIOD_8CLK_gc                 = (PERIOD_8CLK_gv << FUSE_PERIOD_gp),  /* 8 cycles (8ms) */
    PERIOD_16CLK_gc                = (PERIOD_16CLK_gv << FUSE_PERIOD_gp),  /* 16 cycles (16ms) */
    PERIOD_32CLK_gc                = (PERIOD_32CLK_gv << FUSE_PERIOD_gp),  /* 32 cycles (32ms) */
    PERIOD_64CLK_gc                = (PERIOD_64CLK_gv << FUSE_PERIOD_gp),  /* 64 cycles (64ms) */
    PERIOD_128CLK_gc               = (PERIOD_128CLK_gv << FUSE_PERIOD_gp),  /* 128 cycles (0.128s) */
    PERIOD_256CLK_gc               = (PERIOD_256CLK_gv << FUSE_PERIOD_gp),  /* 256 cycles (0.256s) */
    PERIOD_512CLK_gc               = (PERIOD_512CLK_gv << FUSE_PERIOD_gp),  /* 512 cycles (0.512s) */
    PERIOD_1KCLK_gc                = (PERIOD_1KCLK_gv << FUSE_PERIOD_gp),  /* 1K cycles (1.0s) */
    PERIOD_2KCLK_gc                = (PERIOD_2KCLK_gv << FUSE_PERIOD_gp),  /* 2K cycles (2.0s) */
    PERIOD_4KCLK_gc                = (PERIOD_4KCLK_gv << FUSE_PERIOD_gp),  /* 4K cycles (4.1s) */
    PERIOD_8KCLK_gc                = (PERIOD_8KCLK_gv << FUSE_PERIOD_gp)   /* 8K cycles (8.2s) */
} PERIOD_t;

/* Reset Pin Configuration select bit group values */
typedef enum RSTPINCFG_VALUES_enum
{
    RSTPINCFG_GPIO_gv              = 0x00,  /* GPIO mode */
    RSTPINCFG_RST_gv               = 0x01   /* Reset mode */
} RSTPINCFG_values_t;

/* Reset Pin Configuration select bit group configurations*/
typedef enum RSTPINCFG_enum
{
    RSTPINCFG_GPIO_gc              = (RSTPINCFG_GPIO_gv << FUSE_RSTPINCFG_bp),  /* GPIO mode */
    RSTPINCFG_RST_gc               = (RSTPINCFG_RST_gv << FUSE_RSTPINCFG_bp)   /* Reset mode */
} RSTPINCFG_t;

/* BOD Sample Frequency select bit group values */
typedef enum SAMPFREQ_VALUES_enum
{
    SAMPFREQ_1KHZ_gv               = 0x00,  /* 1kHz sampling frequency */
    SAMPFREQ_125HZ_gv              = 0x01   /* 125Hz sampling frequency */
} SAMPFREQ_values_t;

/* BOD Sample Frequency select bit group configurations*/
typedef enum SAMPFREQ_enum
{
    SAMPFREQ_1KHZ_gc               = (SAMPFREQ_1KHZ_gv << FUSE_SAMPFREQ_bp),  /* 1kHz sampling frequency */
    SAMPFREQ_125HZ_gc              = (SAMPFREQ_125HZ_gv << FUSE_SAMPFREQ_bp)   /* 125Hz sampling frequency */
} SAMPFREQ_t;

/* BOD Operation in Sleep Mode select bit group values */
typedef enum SLEEP_VALUES_enum
{
    SLEEP_DIS_gv                   = 0x00,  /* Disabled */
    SLEEP_ENABLED_gv               = 0x01,  /* Enabled */
    SLEEP_SAMPLED_gv               = 0x02   /* Sampled */
} SLEEP_values_t;

/* BOD Operation in Sleep Mode select bit group configurations*/
typedef enum SLEEP_enum
{
    SLEEP_DIS_gc                   = (SLEEP_DIS_gv << FUSE_SLEEP_gp),  /* Disabled */
    SLEEP_ENABLED_gc               = (SLEEP_ENABLED_gv << FUSE_SLEEP_gp),  /* Enabled */
    SLEEP_SAMPLED_gc               = (SLEEP_SAMPLED_gv << FUSE_SLEEP_gp)   /* Sampled */
} SLEEP_t;

/* Startup Time select bit group values */
typedef enum SUT_VALUES_enum
{
    SUT_0MS_gv                     = 0x00,  /* 0 ms */
    SUT_1MS_gv                     = 0x01,  /* 1 ms */
    SUT_2MS_gv                     = 0x02,  /* 2 ms */
    SUT_4MS_gv                     = 0x03,  /* 4 ms */
    SUT_8MS_gv                     = 0x04,  /* 8 ms */
    SUT_16MS_gv                    = 0x05,  /* 16 ms */
    SUT_32MS_gv                    = 0x06,  /* 32 ms */
    SUT_64MS_gv                    = 0x07   /* 64 ms */
} SUT_values_t;

/* Startup Time select bit group configurations*/
typedef enum SUT_enum
{
    SUT_0MS_gc                     = (SUT_0MS_gv << FUSE_SUT_gp),  /* 0 ms */
    SUT_1MS_gc                     = (SUT_1MS_gv << FUSE_SUT_gp),  /* 1 ms */
    SUT_2MS_gc                     = (SUT_2MS_gv << FUSE_SUT_gp),  /* 2 ms */
    SUT_4MS_gc                     = (SUT_4MS_gv << FUSE_SUT_gp),  /* 4 ms */
    SUT_8MS_gc                     = (SUT_8MS_gv << FUSE_SUT_gp),  /* 8 ms */
    SUT_16MS_gc                    = (SUT_16MS_gv << FUSE_SUT_gp),  /* 16 ms */
    SUT_32MS_gc                    = (SUT_32MS_gv << FUSE_SUT_gp),  /* 32 ms */
    SUT_64MS_gc                    = (SUT_64MS_gv << FUSE_SUT_gp)   /* 64 ms */
} SUT_t;

/* Watchdog Window Timeout Period select bit group values */
typedef enum WINDOW_VALUES_enum
{
    WINDOW_OFF_gv                  = 0x00,  /* Off */
    WINDOW_8CLK_gv                 = 0x01,  /* 8 cycles (8ms) */
    WINDOW_16CLK_gv                = 0x02,  /* 16 cycles (16ms) */
    WINDOW_32CLK_gv                = 0x03,  /* 32 cycles (32ms) */
    WINDOW_64CLK_gv                = 0x04,  /* 64 cycles (64ms) */
    WINDOW_128CLK_gv               = 0x05,  /* 128 cycles (0.128s) */
    WINDOW_256CLK_gv               = 0x06,  /* 256 cycles (0.256s) */
    WINDOW_512CLK_gv               = 0x07,  /* 512 cycles (0.512s) */
    WINDOW_1KCLK_gv                = 0x08,  /* 1K cycles (1.0s) */
    WINDOW_2KCLK_gv                = 0x09,  /* 2K cycles (2.0s) */
    WINDOW_4KCLK_gv                = 0x0A,  /* 4K cycles (4.1s) */
    WINDOW_8KCLK_gv                = 0x0B   /* 8K cycles (8.2s) */
} WINDOW_values_t;

/* Watchdog Window Timeout Period select bit group configurations*/
typedef enum WINDOW_enum
{
    WINDOW_OFF_gc                  = (WINDOW_OFF_gv << FUSE_WINDOW_gp),  /* Off */
    WINDOW_8CLK_gc                 = (WINDOW_8CLK_gv << FUSE_WINDOW_gp),  /* 8 cycles (8ms) */
    WINDOW_16CLK_gc                = (WINDOW_16CLK_gv << FUSE_WINDOW_gp),  /* 16 cycles (16ms) */
    WINDOW_32CLK_gc                = (WINDOW_32CLK_gv << FUSE_WINDOW_gp),  /* 32 cycles (32ms) */
    WINDOW_64CLK_gc                = (WINDOW_64CLK_gv << FUSE_WINDOW_gp),  /* 64 cycles (64ms) */
    WINDOW_128CLK_gc               = (WINDOW_128CLK_gv << FUSE_WINDOW_gp),  /* 128 cycles (0.128s) */
    WINDOW_256CLK_gc               = (WINDOW_256CLK_gv << FUSE_WINDOW_gp),  /* 256 cycles (0.256s) */
    WINDOW_512CLK_gc               = (WINDOW_512CLK_gv << FUSE_WINDOW_gp),  /* 512 cycles (0.512s) */
    WINDOW_1KCLK_gc                = (WINDOW_1KCLK_gv << FUSE_WINDOW_gp),  /* 1K cycles (1.0s) */
    WINDOW_2KCLK_gc                = (WINDOW_2KCLK_gv << FUSE_WINDOW_gp),  /* 2K cycles (2.0s) */
    WINDOW_4KCLK_gc                = (WINDOW_4KCLK_gv << FUSE_WINDOW_gp),  /* 4K cycles (4.1s) */
    WINDOW_8KCLK_gc                = (WINDOW_8KCLK_gv << FUSE_WINDOW_gp)   /* 8K cycles (8.2s) */
} WINDOW_t;


/* Non-volatile Memory Controller */
typedef struct NVMCTRL_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t CTRLB;  /* Control B */
    register8_t STATUS;  /* Status */
    register8_t INTCTRL;  /* Interrupt Control */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t reserved_1[1];
    _WORDREGISTER(DATA);  /* Data */
    _WORDREGISTER(ADDR);  /* Address */
    register8_t reserved_2[6];
} NVMCTRL_t;

/* Command select bit group values */
typedef enum NVMCTRL_CMD_VALUES_enum
{
    NVMCTRL_CMD_NONE_gv            = 0x00,  /* No Command */
    NVMCTRL_CMD_PAGEWRITE_gv       = 0x01,  /* Write page */
    NVMCTRL_CMD_PAGEERASE_gv       = 0x02,  /* Erase page */
    NVMCTRL_CMD_PAGEERASEWRITE_gv  = 0x03,  /* Erase and write page */
    NVMCTRL_CMD_PAGEBUFCLR_gv      = 0x04,  /* Page buffer clear */
    NVMCTRL_CMD_CHIPERASE_gv       = 0x05,  /* Chip erase */
    NVMCTRL_CMD_EEERASE_gv         = 0x06,  /* EEPROM erase */
    NVMCTRL_CMD_FUSEWRITE_gv       = 0x07   /* Write fuse (PDI only) */
} NVMCTRL_CMD_values_t;

/* Command select bit group configurations*/
typedef enum NVMCTRL_CMD_enum
{
    NVMCTRL_CMD_NONE_gc            = (NVMCTRL_CMD_NONE_gv << NVMCTRL_CMD_gp),  /* No Command */
    NVMCTRL_CMD_PAGEWRITE_gc       = (NVMCTRL_CMD_PAGEWRITE_gv << NVMCTRL_CMD_gp),  /* Write page */
    NVMCTRL_CMD_PAGEERASE_gc       = (NVMCTRL_CMD_PAGEERASE_gv << NVMCTRL_CMD_gp),  /* Erase page */
    NVMCTRL_CMD_PAGEERASEWRITE_gc  = (NVMCTRL_CMD_PAGEERASEWRITE_gv << NVMCTRL_CMD_gp),  /* Erase and write page */
    NVMCTRL_CMD_PAGEBUFCLR_gc      = (NVMCTRL_CMD_PAGEBUFCLR_gv << NVMCTRL_CMD_gp),  /* Page buffer clear */
    NVMCTRL_CMD_CHIPERASE_gc       = (NVMCTRL_CMD_CHIPERASE_gv << NVMCTRL_CMD_gp),  /* Chip erase */
    NVMCTRL_CMD_EEERASE_gc         = (NVMCTRL_CMD_EEERASE_gv << NVMCTRL_CMD_gp),  /* EEPROM erase */
    NVMCTRL_CMD_FUSEWRITE_gc       = (NVMCTRL_CMD_FUSEWRITE_gv << NVMCTRL_CMD_gp)   /* Write fuse (PDI only) */
} NVMCTRL_CMD_t;

/* End of extract */

#endif //_XT_IO_NVM_H_
