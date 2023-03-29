#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>


/****************************************/
//This firmware uses Timer1 to output 2 phase correct PWM signals.
//The output A is a fixed PWM at 1/5 ratio
//The output B is a variable PWM from 1/16 to 6/16 and back to 1/16.
//If connected to a LED, we would see it fading in and out periodically.


volatile unsigned char counter = 0;
//Period definition: 15625 kHz
const unsigned int PWM_PERIOD = F_CPU / 64 / 16;

ISR(TIMER1_OVF_vect)
{
    //Toggle PB0, to have an external marker for PWM periods
    PORTB ^= 0x01;
    //increment the period counter and update the new PWM ratio for OC1B
    unsigned char c = counter;
    if ((c % 8) < 4)
        OCR1B += PWM_PERIOD / 8;
    else
        OCR1B -= PWM_PERIOD / 8;
    counter = c + 1;
}

void main()
{
    //Port config, PB0 as output, initially low
    DDRB = 0x01;
    PORTB = 0x00;

    //Timer1 configuration : We use the mode 10 (PWM, Phase Correct with TOP=ICR1)
    //Set the PWM period
    ICR1 = PWM_PERIOD - 1;
    //The output compare A ratio is fixed at 20%
    OCR1A = (PWM_PERIOD * 2) / 5 - 1;
    //The output compare B ratio is initially set at 12.5%
    OCR1B = PWM_PERIOD / 8 - 1;
    //Enable the interrupt flag for Timer1 overflow
    TIMSK1 = _BV(TOIE1);
    //COMA and COMB set to non-inverting, Timer1 in mode 10
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
    //Timer1 enabled, configured in mode 10 with x64 prescaling factor
    TCCR1B = _BV(WGM13) | _BV(CS11) | _BV(CS10);

    //Enable interrupts and enter the infinite loop
    sei();
    while(1);
}
