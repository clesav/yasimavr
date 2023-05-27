#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>


/****************************************/

ISR(TCB0_INT_vect)
{
    //Toggle pin PB0
    PORTB.OUTTGL = 0x01;

    //Clear the interrupt flag of TCB0
    TCB0.INTFLAGS = TCB_CAPT_bm;
}

void main() {
    //Port config, PB0 as output, initially low
    PORTB.DIR = 0x01;
    PORTB.OUT = 0x00;

    //TimerB0 configuration
    //TCB0 configured in periodic interrupt mode
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;

    //The counter period threshold is set to 0.5s
    TCB0.CCMP = F_CPU / 1024 / 2;

    //Enable the TCB0 Capture interrupt vector
    TCB0.INTCTRL = TCB_CAPT_bm;

    //Enable TCB0 to run, fed with the TCA prescaler
    TCB0.CTRLA = TCB_CLKSEL_CLKTCA_gc| TCB_ENABLE_bm;

    //Enable TimerA with a 1024 prescaling factor
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;

    //Infinite loop
    sei();
    while(1);
}
