/*
 * test firmware
 *
 *  Copyright 2023 Clement Savergne <csavergne@yahoo.com>

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
 * A ISR template that simply writes in GPIOR0 the interrupt vector number
 */
#define def_test_vector(v) \
    ISR(v) { \
        GPIOR0 = v ## _num; \
    } \

def_test_vector(INT0_vect);
def_test_vector(INT1_vect);
def_test_vector(PCINT0_vect);
def_test_vector(PCINT1_vect);
def_test_vector(PCINT2_vect);

def_test_vector(WDT_vect);

def_test_vector(TIMER0_OVF_vect);
def_test_vector(TIMER0_COMPA_vect);
def_test_vector(TIMER0_COMPB_vect);

def_test_vector(TIMER1_OVF_vect);
def_test_vector(TIMER1_COMPA_vect);
def_test_vector(TIMER1_COMPB_vect);
def_test_vector(TIMER1_CAPT_vect);

def_test_vector(TIMER2_OVF_vect);
def_test_vector(TIMER2_COMPA_vect);
def_test_vector(TIMER2_COMPB_vect);

def_test_vector(SPI_STC_vect);
def_test_vector(USART_RX_vect);
def_test_vector(USART_UDRE_vect);
def_test_vector(USART_TX_vect);

def_test_vector(ADC_vect);
def_test_vector(EE_READY_vect);
def_test_vector(ANALOG_COMP_vect);
def_test_vector(TWI_vect);
def_test_vector(SPM_READY_vect);

int main()
{
    sei();
    while (1);
}

const PROGMEM char message[] = "ConstChar";
char message2[] = "Char";
unsigned int static_int;

char eeprom_data[] EEMEM = "EEPROM";

FUSES = { 0xAA, 0xD9, 0xFF };

uint8_t lockbit LOCKMEM = 0xEF;
