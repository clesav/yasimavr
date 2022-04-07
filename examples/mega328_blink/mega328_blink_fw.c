#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>


/****************************************/

ISR(TIMER1_COMPA_vect)
{
	//Toggle pin PB0
    PORTB ^= 0x01;
}

void main() {
    //Port config, PB0 as output, initially low
    DDRB = 0x01;
    PORTB = 0x00;
	
    //Timer1 configuration
	
	//The counter period threshold is set to 0.5s
	OCR1A = F_CPU / 1024 / 2;
	//Timer1 enabled, configured in CTC mode with 1024 prescaling factor
    TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
	//Enable the interrupt vector for Timer1 output compare A 
    TIMSK1 = _BV(OCIE1A);
    
	//Infinite loop
    sei();
    while(1);
}
