#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>


/****************************************/

volatile uint8_t value_seconds;

ISR(PCINT0_vect)
{
    if (!(PINB & _BV(PINB7))) {
        //The pin PB7 is low, start the stopwatch

        //Reset the counter
        value_seconds = 0;

        //Reset the displayed value
        PORTB = (PORTB & 0x80) | 0;

        //Reset the counter of Timer1
        TCNT1 = 0;
        //Timer1 enabled, configured in CTC mode with 1024 prescaling factor
        TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
        //Ensure the compare A flag is reset
        TIFR1 = _BV(OCIE1A);

    } else {
        //The pin PB7 is high

        //Stop the counting by disabling the timer clock source
        TCCR1B = 0;
    }
}

ISR(TIMER1_COMPA_vect)
{
    //On timer1 interrupt, increment the counter value
    //and output it on PB0-6
    uint8_t v = value_seconds;
    if (v < 99) {
        ++v;
        value_seconds = v;
        PORTB = (PORTB & 0x80) | v;
    }
}

void main() {
    //GPIO port configuration, PB0-6 as output, PB7 as input with a pull-up
    DDRB = 0x7F;
    PORTB = 0x80;

    //Enable external interrupt on pin change for PB7 (=PCINT7)
    PCMSK0 = _BV(PCINT7);
    PCICR = _BV(PCIE0);

    //Timer1 configuration
    //Set the maximum value for the counter,  period threshold is set to 0.1s
    OCR1A = F_CPU / 1024 / 10;
    //Enable the compare A interrupt
    TIMSK1 = _BV(OCIE1A);

    //Enable the interrupt and enter in an infinite loop
    sei();
    while(1);
}
