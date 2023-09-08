from yasimavr.cli import simrun

simrun.main([
    '--mcu', 'atmega328',
    #Frequency of 16 MHz
    '--frequency', '16000000',
    #Stop the simulation after 32M cycles (= 2secs)
    '--cycles', '32000000',
    #Maximum verbosity
    '--verbose', '4',
    #Dump the final simulation state
    '--dump', 'mega328_PWM_dump.txt',
    #Record the traces
    '--output', 'mega328_PWM_record.vcd',
    #Tracing the state changes of pin PB0
    '--trace', 'pin', 'PB0',
    #Tracing the interrupt vector 13 (TIMER1_COMPA)
    '--trace', 'vector', '13',
    #Tracing the OutputCompareA&B state changes of Timer1
    '--trace', 'signal', 'TC1/id=2,ix=0,size=1,name=OCA',
    '--trace', 'signal', 'TC1/id=2,ix=1,size=1,name=OCB',
    #Tracing the value of the static variable 'counter'
    '--trace', 'data', '0x101/counter',
    #Firmware to load
    'mega328_pwm_fw.elf'
])
