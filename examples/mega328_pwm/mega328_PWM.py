from yasimavr.cli import simrun

simrun.main([
    '--mcu', 'atmega328',
    '--frequency', '16000000',
    '--cycles', '32000000',
    '--verbose', '4',
    '--dump', 'mega328_PWM_dump.txt',
    '--output', 'mega328_PWM_record.vcd',
    '--trace', 'pin', 'PB0',
    '--trace', 'vector', '13',
    '--trace', 'signal', 'TC_1/2/0/1/OCA',
    '--trace', 'signal', 'TC_1/2/1/1/OCB',
    'mega328_pwm_fw.elf'
])
