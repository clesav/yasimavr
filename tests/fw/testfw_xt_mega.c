/*
 * test firmware
 *
 *  Copyright 2024 Clement Savergne <csavergne@yahoo.com>

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

#define F_CPU 1000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/signature.h>


/*
 * A ISR template that simply :
   - writes in GPIO0 the interrupt vector number
   - writes in GPIOR1 the interrupt flag register
   - clears the interrupt flag register
 */
#define def_test_vector(v, flag_reg, mask) \
    ISR(v) { \
        GPIOR0 = v ## _num; \
        uint8_t f = flag_reg; \
        GPIOR1 = f; \
        flag_reg = mask; \
    } \

def_test_vector(PORTA_PORT_vect, PORTA_INTFLAGS, 0xFF);
def_test_vector(PORTB_PORT_vect, PORTB_INTFLAGS, 0xFF);
def_test_vector(PORTC_PORT_vect, PORTC_INTFLAGS, 0xFF);
def_test_vector(PORTD_PORT_vect, PORTD_INTFLAGS, 0xFF);
#ifdef PORTE_PORT_vect
def_test_vector(PORTE_PORT_vect, PORTE_INTFLAGS, 0xFF);
#endif
def_test_vector(PORTF_PORT_vect, PORTF_INTFLAGS, 0xFF);
def_test_vector(RTC_CNT_vect, RTC_INTFLAGS, 0x03);
def_test_vector(ADC0_RESRDY_vect, ADC0_INTFLAGS, 0x01);
def_test_vector(AC0_AC_vect, AC0_STATUS, AC_CMP_bm);
def_test_vector(SPI0_INT_vect, SPI0_INTFLAGS, SPI_IF_bm);
def_test_vector(TCB0_INT_vect, TCB0_INTFLAGS, TCB_CAPT_bm);
def_test_vector(TCB1_INT_vect, TCB0_INTFLAGS, TCB_CAPT_bm);
def_test_vector(TCB2_INT_vect, TCB0_INTFLAGS, TCB_CAPT_bm);
#ifdef TCB3_INT_vect
def_test_vector(TCB3_INT_vect, TCB0_INTFLAGS, TCB_CAPT_bm);
#endif


ISR(CRCSCAN_NMI_vect) {
    GPIOR0 = CRCSCAN_NMI_vect_num;
}

int main()
{
    sei();
    while (1);
}

//The following defines some constants to ensures the sections
//appear in the ELF file

const char message[] = "ConstChar"; //for .rodata
char message2[] = "StaticChar"; //for .data
char eeprom_data[] EEMEM = "EEPROM"; //for .eeprom

/*FUSES = { //for .fuse
    .WDTCFG = 0x00,
    .BODCFG = 0x00,
    .OSCCFG = 0x81,
    .SYSCFG0 = 0x00,
    .SYSCFG1 = 0x01,
    .APPEND = 0x00,
    .BOOTEND = 0x00
};*/

uint8_t lockbit LOCKMEM = 0xC5;

char userrow[] __attribute__((__used__, __section__ (".user_signatures"))) =
    "UserSig";
