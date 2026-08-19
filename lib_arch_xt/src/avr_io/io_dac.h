/*
 * io_dac.h
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

#ifndef _XT_IO_DAC_H_
#define _XT_IO_DAC_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the DAC peripheral for the following device models:
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

/* DAC - Digital to Analog Converter */
/* DAC.CTRLA  bit masks and bit positions */
#define DAC_ENABLE_bm  0x01  /* DAC Enable bit mask. */
#define DAC_ENABLE_bp  0  /* DAC Enable bit position. */
#define DAC_OUTEN_bm  0x40  /* Output Buffer Enable bit mask. */
#define DAC_OUTEN_bp  6  /* Output Buffer Enable bit position. */
#define DAC_RUNSTDBY_bm  0x80  /* Run in Standby Mode bit mask. */
#define DAC_RUNSTDBY_bp  7  /* Run in Standby Mode bit position. */


/* Digital to Analog Converter */
typedef struct DAC_struct
{
    register8_t CTRLA;  /* Control Register A */
    register8_t DATA;  /* DATA Register */
    register8_t reserved_1[2];
} DAC_t;

/* End of extract */

#endif //_XT_IO_DAC_H_
