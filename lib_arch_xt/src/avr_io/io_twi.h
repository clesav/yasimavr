/*
 * io_twi.h
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

#ifndef _XT_IO_TWI_H_
#define _XT_IO_TWI_H_

#include "../arch_xt_io_utils.h"


//=======================================================================================
/*
 * Extract from AVR IO includes for the TWI peripheral for the following device models:
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


/* TWI - Two-Wire Interface */
/* TWI.CTRLA  bit masks and bit positions */
#define TWI_FMPEN_bm  0x02  /* Fast-mode Plus Enable bit mask. */
#define TWI_FMPEN_bp  1  /* Fast-mode Plus Enable bit position. */
#define TWI_SDAHOLD_gm  0x0C  /* SDA Hold Time group mask. */
#define TWI_SDAHOLD_gp  2  /* SDA Hold Time group position. */
#define TWI_SDAHOLD_0_bm  (1<<2)  /* SDA Hold Time bit 0 mask. */
#define TWI_SDAHOLD_0_bp  2  /* SDA Hold Time bit 0 position. */
#define TWI_SDAHOLD_1_bm  (1<<3)  /* SDA Hold Time bit 1 mask. */
#define TWI_SDAHOLD_1_bp  3  /* SDA Hold Time bit 1 position. */
#define TWI_SDASETUP_bm  0x10  /* SDA Setup Time bit mask. */
#define TWI_SDASETUP_bp  4  /* SDA Setup Time bit position. */

/* TWI.DUALCTRL  bit masks and bit positions */
#define TWI_ENABLE_bm  0x01  /* Dual Control Enable bit mask. */
#define TWI_ENABLE_bp  0  /* Dual Control Enable bit position. */
/* TWI_FMPEN  is already defined. */
/* TWI_SDAHOLD  is already defined. */

/* TWI.DBGCTRL  bit masks and bit positions */
#define TWI_DBGRUN_bm  0x01  /* Debug Run bit mask. */
#define TWI_DBGRUN_bp  0  /* Debug Run bit position. */

/* TWI.MCTRLA  bit masks and bit positions */
/* TWI_ENABLE  is already defined. */
#define TWI_SMEN_bm  0x02  /* Smart Mode Enable bit mask. */
#define TWI_SMEN_bp  1  /* Smart Mode Enable bit position. */
#define TWI_TIMEOUT_gm  0x0C  /* Inactive Bus Timeout group mask. */
#define TWI_TIMEOUT_gp  2  /* Inactive Bus Timeout group position. */
#define TWI_TIMEOUT_0_bm  (1<<2)  /* Inactive Bus Timeout bit 0 mask. */
#define TWI_TIMEOUT_0_bp  2  /* Inactive Bus Timeout bit 0 position. */
#define TWI_TIMEOUT_1_bm  (1<<3)  /* Inactive Bus Timeout bit 1 mask. */
#define TWI_TIMEOUT_1_bp  3  /* Inactive Bus Timeout bit 1 position. */
#define TWI_QCEN_bm  0x10  /* Quick Command Enable bit mask. */
#define TWI_QCEN_bp  4  /* Quick Command Enable bit position. */
#define TWI_WIEN_bm  0x40  /* Write Interrupt Enable bit mask. */
#define TWI_WIEN_bp  6  /* Write Interrupt Enable bit position. */
#define TWI_RIEN_bm  0x80  /* Read Interrupt Enable bit mask. */
#define TWI_RIEN_bp  7  /* Read Interrupt Enable bit position. */

/* TWI.MCTRLB  bit masks and bit positions */
#define TWI_MCMD_gm  0x03  /* Command group mask. */
#define TWI_MCMD_gp  0  /* Command group position. */
#define TWI_MCMD_0_bm  (1<<0)  /* Command bit 0 mask. */
#define TWI_MCMD_0_bp  0  /* Command bit 0 position. */
#define TWI_MCMD_1_bm  (1<<1)  /* Command bit 1 mask. */
#define TWI_MCMD_1_bp  1  /* Command bit 1 position. */
#define TWI_ACKACT_bm  0x04  /* Acknowledge Action bit mask. */
#define TWI_ACKACT_bp  2  /* Acknowledge Action bit position. */
#define TWI_FLUSH_bm  0x08  /* Flush bit mask. */
#define TWI_FLUSH_bp  3  /* Flush bit position. */

/* TWI.MSTATUS  bit masks and bit positions */
#define TWI_BUSSTATE_gm  0x03  /* Bus State group mask. */
#define TWI_BUSSTATE_gp  0  /* Bus State group position. */
#define TWI_BUSSTATE_0_bm  (1<<0)  /* Bus State bit 0 mask. */
#define TWI_BUSSTATE_0_bp  0  /* Bus State bit 0 position. */
#define TWI_BUSSTATE_1_bm  (1<<1)  /* Bus State bit 1 mask. */
#define TWI_BUSSTATE_1_bp  1  /* Bus State bit 1 position. */
#define TWI_BUSERR_bm  0x04  /* Bus Error bit mask. */
#define TWI_BUSERR_bp  2  /* Bus Error bit position. */
#define TWI_ARBLOST_bm  0x08  /* Arbitration Lost bit mask. */
#define TWI_ARBLOST_bp  3  /* Arbitration Lost bit position. */
#define TWI_RXACK_bm  0x10  /* Received Acknowledge bit mask. */
#define TWI_RXACK_bp  4  /* Received Acknowledge bit position. */
#define TWI_CLKHOLD_bm  0x20  /* Clock Hold bit mask. */
#define TWI_CLKHOLD_bp  5  /* Clock Hold bit position. */
#define TWI_WIF_bm  0x40  /* Write Interrupt Flag bit mask. */
#define TWI_WIF_bp  6  /* Write Interrupt Flag bit position. */
#define TWI_RIF_bm  0x80  /* Read Interrupt Flag bit mask. */
#define TWI_RIF_bp  7  /* Read Interrupt Flag bit position. */

/* TWI.SCTRLA  bit masks and bit positions */
/* TWI_ENABLE  is already defined. */
/* TWI_SMEN  is already defined. */
#define TWI_PMEN_bm  0x04  /* Promiscuous Mode Enable bit mask. */
#define TWI_PMEN_bp  2  /* Promiscuous Mode Enable bit position. */
#define TWI_PIEN_bm  0x20  /* Stop Interrupt Enable bit mask. */
#define TWI_PIEN_bp  5  /* Stop Interrupt Enable bit position. */
#define TWI_APIEN_bm  0x40  /* Address/Stop Interrupt Enable bit mask. */
#define TWI_APIEN_bp  6  /* Address/Stop Interrupt Enable bit position. */
#define TWI_DIEN_bm  0x80  /* Data Interrupt Enable bit mask. */
#define TWI_DIEN_bp  7  /* Data Interrupt Enable bit position. */

/* TWI.SCTRLB  bit masks and bit positions */
#define TWI_SCMD_gm  0x03  /* Command group mask. */
#define TWI_SCMD_gp  0  /* Command group position. */
#define TWI_SCMD_0_bm  (1<<0)  /* Command bit 0 mask. */
#define TWI_SCMD_0_bp  0  /* Command bit 0 position. */
#define TWI_SCMD_1_bm  (1<<1)  /* Command bit 1 mask. */
#define TWI_SCMD_1_bp  1  /* Command bit 1 position. */
/* TWI_ACKACT  is already defined. */

/* TWI.SSTATUS  bit masks and bit positions */
#define TWI_AP_bm  0x01  /* Client Address or Stop bit mask. */
#define TWI_AP_bp  0  /* Client Address or Stop bit position. */
#define TWI_DIR_bm  0x02  /* Read/Write Direction bit mask. */
#define TWI_DIR_bp  1  /* Read/Write Direction bit position. */
/* TWI_BUSERR  is already defined. */
#define TWI_COLL_bm  0x08  /* Collision bit mask. */
#define TWI_COLL_bp  3  /* Collision bit position. */
/* TWI_RXACK  is already defined. */
/* TWI_CLKHOLD  is already defined. */
#define TWI_APIF_bm  0x40  /* Address/Stop Interrupt Flag bit mask. */
#define TWI_APIF_bp  6  /* Address/Stop Interrupt Flag bit position. */
#define TWI_DIF_bm  0x80  /* Data Interrupt Flag bit mask. */
#define TWI_DIF_bp  7  /* Data Interrupt Flag bit position. */

/* TWI.SADDRMASK  bit masks and bit positions */
#define TWI_ADDREN_bm  0x01  /* Address Enable bit mask. */
#define TWI_ADDREN_bp  0  /* Address Enable bit position. */
#define TWI_ADDRMASK_gm  0xFE  /* Address Mask group mask. */
#define TWI_ADDRMASK_gp  1  /* Address Mask group position. */
#define TWI_ADDRMASK_0_bm  (1<<1)  /* Address Mask bit 0 mask. */
#define TWI_ADDRMASK_0_bp  1  /* Address Mask bit 0 position. */
#define TWI_ADDRMASK_1_bm  (1<<2)  /* Address Mask bit 1 mask. */
#define TWI_ADDRMASK_1_bp  2  /* Address Mask bit 1 position. */
#define TWI_ADDRMASK_2_bm  (1<<3)  /* Address Mask bit 2 mask. */
#define TWI_ADDRMASK_2_bp  3  /* Address Mask bit 2 position. */
#define TWI_ADDRMASK_3_bm  (1<<4)  /* Address Mask bit 3 mask. */
#define TWI_ADDRMASK_3_bp  4  /* Address Mask bit 3 position. */
#define TWI_ADDRMASK_4_bm  (1<<5)  /* Address Mask bit 4 mask. */
#define TWI_ADDRMASK_4_bp  5  /* Address Mask bit 4 position. */
#define TWI_ADDRMASK_5_bm  (1<<6)  /* Address Mask bit 5 mask. */
#define TWI_ADDRMASK_5_bp  6  /* Address Mask bit 5 position. */
#define TWI_ADDRMASK_6_bm  (1<<7)  /* Address Mask bit 6 mask. */
#define TWI_ADDRMASK_6_bp  7  /* Address Mask bit 6 position. */


/* Two-Wire Interface */
typedef struct TWI_struct
{
    register8_t CTRLA;  /* Control A */
    register8_t DUALCTRL;  /* Dual Control */
    register8_t DBGCTRL;  /* Debug Control Register */
    register8_t MCTRLA;  /* Host Control A */
    register8_t MCTRLB;  /* Host Control B */
    register8_t MSTATUS;  /* Host Status */
    register8_t MBAUD;  /* Host Baud Rate Control */
    register8_t MADDR;  /* Host Address */
    register8_t MDATA;  /* Host Data */
    register8_t SCTRLA;  /* Client Control A */
    register8_t SCTRLB;  /* Client Control B */
    register8_t SSTATUS;  /* Client Status */
    register8_t SADDR;  /* Client Address */
    register8_t SDATA;  /* Client Data */
    register8_t SADDRMASK;  /* Client Address Mask */
    register8_t reserved_1[1];
} TWI_t;

/* Acknowledge Action select bit group values */
typedef enum TWI_ACKACT_VALUES_enum
{
    TWI_ACKACT_ACK_gv              = 0x00,  /* Send ACK */
    TWI_ACKACT_NACK_gv             = 0x01   /* Send NACK */
} TWI_ACKACT_values_t;

/* Acknowledge Action select bit group configurations*/
typedef enum TWI_ACKACT_enum
{
    TWI_ACKACT_ACK_gc              = (TWI_ACKACT_ACK_gv << TWI_ACKACT_bp),  /* Send ACK */
    TWI_ACKACT_NACK_gc             = (TWI_ACKACT_NACK_gv << TWI_ACKACT_bp)   /* Send NACK */
} TWI_ACKACT_t;

/* Client Address or Stop select bit group values */
typedef enum TWI_AP_VALUES_enum
{
    TWI_AP_STOP_gv                 = 0x00,  /* Stop condition generated APIF */
    TWI_AP_ADR_gv                  = 0x01   /* Address detection generated APIF */
} TWI_AP_values_t;

/* Client Address or Stop select bit group configurations*/
typedef enum TWI_AP_enum
{
    TWI_AP_STOP_gc                 = (TWI_AP_STOP_gv << TWI_AP_bp),  /* Stop condition generated APIF */
    TWI_AP_ADR_gc                  = (TWI_AP_ADR_gv << TWI_AP_bp)   /* Address detection generated APIF */
} TWI_AP_t;

/* Bus State select bit group values */
typedef enum TWI_BUSSTATE_VALUES_enum
{
    TWI_BUSSTATE_UNKNOWN_gv        = 0x00,  /* Unknown Bus State */
    TWI_BUSSTATE_IDLE_gv           = 0x01,  /* Bus is Idle */
    TWI_BUSSTATE_OWNER_gv          = 0x02,  /* This Module Controls The Bus */
    TWI_BUSSTATE_BUSY_gv           = 0x03   /* The Bus is Busy */
} TWI_BUSSTATE_values_t;

/* Bus State select bit group configurations*/
typedef enum TWI_BUSSTATE_enum
{
    TWI_BUSSTATE_UNKNOWN_gc        = (TWI_BUSSTATE_UNKNOWN_gv << TWI_BUSSTATE_gp),  /* Unknown Bus State */
    TWI_BUSSTATE_IDLE_gc           = (TWI_BUSSTATE_IDLE_gv << TWI_BUSSTATE_gp),  /* Bus is Idle */
    TWI_BUSSTATE_OWNER_gc          = (TWI_BUSSTATE_OWNER_gv << TWI_BUSSTATE_gp),  /* This Module Controls The Bus */
    TWI_BUSSTATE_BUSY_gc           = (TWI_BUSSTATE_BUSY_gv << TWI_BUSSTATE_gp)   /* The Bus is Busy */
} TWI_BUSSTATE_t;

/* Command select bit group values */
typedef enum TWI_MCMD_VALUES_enum
{
    TWI_MCMD_NOACT_gv              = 0x00,  /* No Action */
    TWI_MCMD_REPSTART_gv           = 0x01,  /* Issue Repeated Start Condition */
    TWI_MCMD_RECVTRANS_gv          = 0x02,  /* Receive or Transmit Data, depending on DIR */
    TWI_MCMD_STOP_gv               = 0x03   /* Issue Stop Condition */
} TWI_MCMD_values_t;

/* Command select bit group configurations*/
typedef enum TWI_MCMD_enum
{
    TWI_MCMD_NOACT_gc              = (TWI_MCMD_NOACT_gv << TWI_MCMD_gp),  /* No Action */
    TWI_MCMD_REPSTART_gc           = (TWI_MCMD_REPSTART_gv << TWI_MCMD_gp),  /* Issue Repeated Start Condition */
    TWI_MCMD_RECVTRANS_gc          = (TWI_MCMD_RECVTRANS_gv << TWI_MCMD_gp),  /* Receive or Transmit Data, depending on DIR */
    TWI_MCMD_STOP_gc               = (TWI_MCMD_STOP_gv << TWI_MCMD_gp)   /* Issue Stop Condition */
} TWI_MCMD_t;

/* Command select bit group values */
typedef enum TWI_SCMD_VALUES_enum
{
    TWI_SCMD_NOACT_gv              = 0x00,  /* No Action */
    TWI_SCMD_COMPTRANS_gv          = 0x02,  /* Used To Complete a Transaction */
    TWI_SCMD_RESPONSE_gv           = 0x03   /* Used in Response to Address/Data Interrupt */
} TWI_SCMD_values_t;

/* Command select bit group configurations*/
typedef enum TWI_SCMD_enum
{
    TWI_SCMD_NOACT_gc              = (TWI_SCMD_NOACT_gv << TWI_SCMD_gp),  /* No Action */
    TWI_SCMD_COMPTRANS_gc          = (TWI_SCMD_COMPTRANS_gv << TWI_SCMD_gp),  /* Used To Complete a Transaction */
    TWI_SCMD_RESPONSE_gc           = (TWI_SCMD_RESPONSE_gv << TWI_SCMD_gp)   /* Used in Response to Address/Data Interrupt */
} TWI_SCMD_t;

/* SDA Hold Time select bit group values */
typedef enum TWI_SDAHOLD_VALUES_enum
{
    TWI_SDAHOLD_OFF_gv             = 0x00,  /* SDA hold time off */
    TWI_SDAHOLD_50NS_gv            = 0x01,  /* Typical 50ns hold time */
    TWI_SDAHOLD_300NS_gv           = 0x02,  /* Typical 300ns hold time */
    TWI_SDAHOLD_500NS_gv           = 0x03   /* Typical 500ns hold time */
} TWI_SDAHOLD_values_t;

/* SDA Hold Time select bit group configurations*/
typedef enum TWI_SDAHOLD_enum
{
    TWI_SDAHOLD_OFF_gc             = (TWI_SDAHOLD_OFF_gv << TWI_SDAHOLD_gp),  /* SDA hold time off */
    TWI_SDAHOLD_50NS_gc            = (TWI_SDAHOLD_50NS_gv << TWI_SDAHOLD_gp),  /* Typical 50ns hold time */
    TWI_SDAHOLD_300NS_gc           = (TWI_SDAHOLD_300NS_gv << TWI_SDAHOLD_gp),  /* Typical 300ns hold time */
    TWI_SDAHOLD_500NS_gc           = (TWI_SDAHOLD_500NS_gv << TWI_SDAHOLD_gp)   /* Typical 500ns hold time */
} TWI_SDAHOLD_t;

/* SDA Setup Time select bit group values */
typedef enum TWI_SDASETUP_VALUES_enum
{
    TWI_SDASETUP_4CYC_gv           = 0x00,  /* SDA setup time is 4 clock cycles */
    TWI_SDASETUP_8CYC_gv           = 0x01   /* SDA setup time is 8 clock cycles */
} TWI_SDASETUP_values_t;

/* SDA Setup Time select bit group configurations*/
typedef enum TWI_SDASETUP_enum
{
    TWI_SDASETUP_4CYC_gc           = (TWI_SDASETUP_4CYC_gv << TWI_SDASETUP_bp),  /* SDA setup time is 4 clock cycles */
    TWI_SDASETUP_8CYC_gc           = (TWI_SDASETUP_8CYC_gv << TWI_SDASETUP_bp)   /* SDA setup time is 8 clock cycles */
} TWI_SDASETUP_t;

/* Inactive Bus Timeout select bit group values */
typedef enum TWI_TIMEOUT_VALUES_enum
{
    TWI_TIMEOUT_DISABLED_gv        = 0x00,  /* Bus Timeout Disabled */
    TWI_TIMEOUT_50US_gv            = 0x01,  /* 50 Microseconds */
    TWI_TIMEOUT_100US_gv           = 0x02,  /* 100 Microseconds */
    TWI_TIMEOUT_200US_gv           = 0x03   /* 200 Microseconds */
} TWI_TIMEOUT_values_t;

/* Inactive Bus Timeout select bit group configurations*/
typedef enum TWI_TIMEOUT_enum
{
    TWI_TIMEOUT_DISABLED_gc        = (TWI_TIMEOUT_DISABLED_gv << TWI_TIMEOUT_gp),  /* Bus Timeout Disabled */
    TWI_TIMEOUT_50US_gc            = (TWI_TIMEOUT_50US_gv << TWI_TIMEOUT_gp),  /* 50 Microseconds */
    TWI_TIMEOUT_100US_gc           = (TWI_TIMEOUT_100US_gv << TWI_TIMEOUT_gp),  /* 100 Microseconds */
    TWI_TIMEOUT_200US_gc           = (TWI_TIMEOUT_200US_gv << TWI_TIMEOUT_gp)   /* 200 Microseconds */
} TWI_TIMEOUT_t;

/* End of extract */

#endif //_XT_IO_TWI_H_
