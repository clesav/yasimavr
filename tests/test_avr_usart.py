# test_avr_usart.py
#
# Copyright 2025 Clement Savergne <csavergne@yahoo.com>
#
# This file is part of yasim-avr.
#
# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.


'''
Automatic tests for the USART simulation model (AVR architecture)
'''


import pytest
from _test_bench_avr import BenchAVR, PATH_TESTFW_M328
import yasimavr.lib.core as corelib
from yasimavr.device_library import load_device
from yasimavr.utils.uart import RawUART, UART_IO


@pytest.fixture
def bench():
    dev_model = load_device('atmega328')
    b = BenchAVR(dev_model, PATH_TESTFW_M328)

    b.pin_txd = b.dev.pins['PD1']
    b.pin_rxd = b.dev.pins['PD0']

    b.pin_txd.set_external_state('U')
    b.pin_rxd.set_external_state('U')

    return b


def test_avr_usart_tx(bench):
    '''This test checks the transmission of a byte
    '''

    raw_uart = RawUART()
    raw_uart.init(bench.loop.cycle_manager())
    raw_uart.set_bit_delay(16)
    bench.pin_txd.attach(raw_uart.lineRXD)
    bench.pin_rxd.attach(raw_uart.lineTXD)
    raw_uart.set_rx_enabled(True)

    USART = bench.dev.USART
    PORTD = bench.dev.PORTD

    PORTD.PORT = 0x02
    PORTD.DDR = 0x02
    USART.UCSRB.TXEN = 1

    USART.UDR = 0x42

    bench.sim_advance(200)

    assert USART.UCSRA.TXC
    USART.UCSRA.TXC = 1
    assert not USART.UCSRA.TXC

    assert raw_uart.rx_available() == 1
    assert raw_uart.read_rx() == 0x42


def test_avr_usart_rx(bench):
    '''This test checks the reception of a byte
    '''

    raw_uart = RawUART()
    raw_uart.init(bench.loop.cycle_manager())
    raw_uart.set_bit_delay(16)
    bench.pin_txd.attach(raw_uart.lineRXD)
    bench.pin_rxd.attach(raw_uart.lineTXD)
    raw_uart.set_rx_enabled(True)

    USART = bench.dev.USART
    PORTD = bench.dev.PORTD

    PORTD.PORT = 0x02
    PORTD.DDR = 0x02
    USART.UCSRB.RXEN = 1

    assert not USART.UCSRA.RXC

    raw_uart.push_tx(0x53)
    bench.sim_advance(200)

    assert USART.UCSRA.RXC
    assert USART.UDR.read() == 0x53
    assert not USART.UCSRA.RXC


def test_avr_usart_tx_io(bench):
    '''This test checks the transmission of a byte using the IO framework
    '''
    
    io_uart = UART_IO(bench.dev_model, 0)

    USART = bench.dev.USART
    PORTD = bench.dev.PORTD

    PORTD.PORT = 0x02
    PORTD.DDR = 0x02
    USART.UCSRB.TXEN = 1

    USART.UDR = 0x79

    bench.sim_advance(200)

    assert USART.UCSRA.TXC
    USART.UCSRA.TXC = 1
    assert not USART.UCSRA.TXC

    assert io_uart.available() == 1
    b = io_uart.read()
    assert ord(b) == 0x79


def test_avr_usart_rx_io(bench):
    '''This test checks the reception of a byte using the IO framework
    '''

    io_uart = UART_IO(bench.dev_model, 0)

    USART = bench.dev.USART
    PORTD = bench.dev.PORTD

    PORTD.PORT = 0x02
    PORTD.DDR = 0x02
    USART.UCSRB.RXEN = 1

    assert not USART.UCSRA.RXC

    io_uart.write(b'U')
    bench.sim_advance(200)

    assert USART.UCSRA.RXC
    assert USART.UDR.read() == ord(b'U')
    assert not USART.UCSRA.RXC
