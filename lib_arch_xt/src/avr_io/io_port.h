/*
 * io_port.h
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

#ifndef _XT_IO_PORT_H_
#define _XT_IO_PORT_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the PORT/VPORT peripherals for the following device models:
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


/* PORT - I/O Ports */
/* PORT.INTFLAGS  bit masks and bit positions */
#define PORT_INT_gm  0xFF  /* Pin Interrupt group mask. */
#define PORT_INT_gp  0  /* Pin Interrupt group position. */
#define PORT_INT_0_bm  (1<<0)  /* Pin Interrupt bit 0 mask. */
#define PORT_INT_0_bp  0  /* Pin Interrupt bit 0 position. */
#define PORT_INT_1_bm  (1<<1)  /* Pin Interrupt bit 1 mask. */
#define PORT_INT_1_bp  1  /* Pin Interrupt bit 1 position. */
#define PORT_INT_2_bm  (1<<2)  /* Pin Interrupt bit 2 mask. */
#define PORT_INT_2_bp  2  /* Pin Interrupt bit 2 position. */
#define PORT_INT_3_bm  (1<<3)  /* Pin Interrupt bit 3 mask. */
#define PORT_INT_3_bp  3  /* Pin Interrupt bit 3 position. */
#define PORT_INT_4_bm  (1<<4)  /* Pin Interrupt bit 4 mask. */
#define PORT_INT_4_bp  4  /* Pin Interrupt bit 4 position. */
#define PORT_INT_5_bm  (1<<5)  /* Pin Interrupt bit 5 mask. */
#define PORT_INT_5_bp  5  /* Pin Interrupt bit 5 position. */
#define PORT_INT_6_bm  (1<<6)  /* Pin Interrupt bit 6 mask. */
#define PORT_INT_6_bp  6  /* Pin Interrupt bit 6 position. */
#define PORT_INT_7_bm  (1<<7)  /* Pin Interrupt bit 7 mask. */
#define PORT_INT_7_bp  7  /* Pin Interrupt bit 7 position. */

/* PORT.PORTCTRL  bit masks and bit positions */
#define PORT_SRL_bm  0x01  /* Slew Rate Limit Enable bit mask. */
#define PORT_SRL_bp  0  /* Slew Rate Limit Enable bit position. */

/* PORT.PIN0CTRL  bit masks and bit positions */
#define PORT_ISC_gm  0x07  /* Input/Sense Configuration group mask. */
#define PORT_ISC_gp  0  /* Input/Sense Configuration group position. */
#define PORT_ISC_0_bm  (1<<0)  /* Input/Sense Configuration bit 0 mask. */
#define PORT_ISC_0_bp  0  /* Input/Sense Configuration bit 0 position. */
#define PORT_ISC_1_bm  (1<<1)  /* Input/Sense Configuration bit 1 mask. */
#define PORT_ISC_1_bp  1  /* Input/Sense Configuration bit 1 position. */
#define PORT_ISC_2_bm  (1<<2)  /* Input/Sense Configuration bit 2 mask. */
#define PORT_ISC_2_bp  2  /* Input/Sense Configuration bit 2 position. */
#define PORT_PULLUPEN_bm  0x08  /* Pullup enable bit mask. */
#define PORT_PULLUPEN_bp  3  /* Pullup enable bit position. */
#define PORT_INVEN_bm  0x80  /* Inverted I/O Enable bit mask. */
#define PORT_INVEN_bp  7  /* Inverted I/O Enable bit position. */

/* PORT.PIN1CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* PORT.PIN2CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* PORT.PIN3CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* PORT.PIN4CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* PORT.PIN5CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* PORT.PIN6CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* PORT.PIN7CTRL  bit masks and bit positions */
/* PORT_ISC  is already defined. */
/* PORT_PULLUPEN  is already defined. */
/* PORT_INVEN  is already defined. */

/* VPORT - Virtual Ports */
/* VPORT.INTFLAGS  bit masks and bit positions */
#define VPORT_INT_gm  0xFF  /* Pin Interrupt group mask. */
#define VPORT_INT_gp  0  /* Pin Interrupt group position. */
#define VPORT_INT0_bm  (1<<0)  /* Pin Interrupt bit 0 mask. */
#define VPORT_INT0_bp  0  /* Pin Interrupt bit 0 position. */
#define VPORT_INT1_bm  (1<<1)  /* Pin Interrupt bit 1 mask. */
#define VPORT_INT1_bp  1  /* Pin Interrupt bit 1 position. */
#define VPORT_INT2_bm  (1<<2)  /* Pin Interrupt bit 2 mask. */
#define VPORT_INT2_bp  2  /* Pin Interrupt bit 2 position. */
#define VPORT_INT3_bm  (1<<3)  /* Pin Interrupt bit 3 mask. */
#define VPORT_INT3_bp  3  /* Pin Interrupt bit 3 position. */
#define VPORT_INT4_bm  (1<<4)  /* Pin Interrupt bit 4 mask. */
#define VPORT_INT4_bp  4  /* Pin Interrupt bit 4 position. */
#define VPORT_INT5_bm  (1<<5)  /* Pin Interrupt bit 5 mask. */
#define VPORT_INT5_bp  5  /* Pin Interrupt bit 5 position. */
#define VPORT_INT6_bm  (1<<6)  /* Pin Interrupt bit 6 mask. */
#define VPORT_INT6_bp  6  /* Pin Interrupt bit 6 position. */
#define VPORT_INT7_bm  (1<<7)  /* Pin Interrupt bit 7 mask. */
#define VPORT_INT7_bp  7  /* Pin Interrupt bit 7 position. */


/* I/O Ports */
typedef struct PORT_struct
{
    register8_t DIR;  /* Data Direction */
    register8_t DIRSET;  /* Data Direction Set */
    register8_t DIRCLR;  /* Data Direction Clear */
    register8_t DIRTGL;  /* Data Direction Toggle */
    register8_t OUT;  /* Output Value */
    register8_t OUTSET;  /* Output Value Set */
    register8_t OUTCLR;  /* Output Value Clear */
    register8_t OUTTGL;  /* Output Value Toggle */
    register8_t IN;  /* Input Value */
    register8_t INTFLAGS;  /* Interrupt Flags */
    register8_t PORTCTRL;  /* Port Control */
    register8_t reserved_1[5];
    register8_t PIN0CTRL;  /* Pin 0 Control */
    register8_t PIN1CTRL;  /* Pin 1 Control */
    register8_t PIN2CTRL;  /* Pin 2 Control */
    register8_t PIN3CTRL;  /* Pin 3 Control */
    register8_t PIN4CTRL;  /* Pin 4 Control */
    register8_t PIN5CTRL;  /* Pin 5 Control */
    register8_t PIN6CTRL;  /* Pin 6 Control */
    register8_t PIN7CTRL;  /* Pin 7 Control */
    register8_t reserved_2[8];
} PORT_t;

/* Input/Sense Configuration select bit group values */
typedef enum PORT_ISC_VALUES_enum
{
    PORT_ISC_INTDISABLE_gv         = 0x00,  /* Interrupt disabled but input buffer enabled */
    PORT_ISC_BOTHEDGES_gv          = 0x01,  /* Sense Both Edges */
    PORT_ISC_RISING_gv             = 0x02,  /* Sense Rising Edge */
    PORT_ISC_FALLING_gv            = 0x03,  /* Sense Falling Edge */
    PORT_ISC_INPUT_DISABLE_gv      = 0x04,  /* Digital Input Buffer disabled */
    PORT_ISC_LEVEL_gv              = 0x05   /* Sense low Level */
} PORT_ISC_values_t;

/* Input/Sense Configuration select bit group configurations*/
typedef enum PORT_ISC_enum
{
    PORT_ISC_INTDISABLE_gc         = (PORT_ISC_INTDISABLE_gv << PORT_ISC_gp),  /* Interrupt disabled but input buffer enabled */
    PORT_ISC_BOTHEDGES_gc          = (PORT_ISC_BOTHEDGES_gv << PORT_ISC_gp),  /* Sense Both Edges */
    PORT_ISC_RISING_gc             = (PORT_ISC_RISING_gv << PORT_ISC_gp),  /* Sense Rising Edge */
    PORT_ISC_FALLING_gc            = (PORT_ISC_FALLING_gv << PORT_ISC_gp),  /* Sense Falling Edge */
    PORT_ISC_INPUT_DISABLE_gc      = (PORT_ISC_INPUT_DISABLE_gv << PORT_ISC_gp),  /* Digital Input Buffer disabled */
    PORT_ISC_LEVEL_gc              = (PORT_ISC_LEVEL_gv << PORT_ISC_gp)   /* Sense low Level */
} PORT_ISC_t;

/* Virtual Ports */
typedef struct VPORT_struct
{
    register8_t DIR;  /* Data Direction */
    register8_t OUT;  /* Output Value */
    register8_t IN;  /* Input Value */
    register8_t INTFLAGS;  /* Interrupt Flags */
} VPORT_t;

/* End of extract */

#endif //_XT_IO_PORT_H_
