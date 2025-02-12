# test_avr_spi.py
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
Automatic tests for the SPI simulation model (AVR architecture)
'''


import pytest
from _test_bench_avr import BenchAVR, PATH_TESTFW_M328
from _test_utils import PinState
import yasimavr.lib.core as corelib
from yasimavr.device_library import load_device
from yasimavr.utils.spi import SPISimpleClient


@pytest.fixture
def bench():
    dev_model = load_device('atmega328')
    b = BenchAVR(dev_model, PATH_TESTFW_M328)
    b.pin_clk = b.dev.pins['PB5']
    b.pin_miso = b.dev.pins['PB4']
    b.pin_mosi = b.dev.pins['PB3']
    b.pin_sel = b.dev.pins['PB2']

    b.pin_clk.set_external_state('D')
    b.pin_miso.set_external_state('D')
    b.pin_mosi.set_external_state('D')
    b.pin_sel.set_external_state('U')

    return b


def test_avr_spi_pin_drive(bench):
    SPI = bench.dev.SPI
    PORTB = bench.dev.PORTB

    PORTB.PORT = 0xFF
    PORTB.DDR = 0xFF

    bench.pin_sel.set_external_state('U')

    assert SPI.SPCR.SPE == False
    assert bench.pin_clk.state() == PinState.High
    assert bench.pin_miso.state() == PinState.High
    assert bench.pin_mosi.state() == PinState.High

    bench.sim_advance(10)
    print('Setting host mode')
    SPI.SPCR.MSTR = True
    SPI.SPCR.SPE = True
    assert bench.pin_clk.state() == PinState.Low
    assert bench.pin_miso.state() == PinState.PullDown
    assert bench.pin_mosi.state() == PinState.Low

    bench.sim_advance(10)
    print('Setting client mode')
    SPI.SPCR.MSTR = False
    assert bench.pin_clk.state() == PinState.PullDown
    assert bench.pin_miso.state() == PinState.PullDown
    assert bench.pin_mosi.state() == PinState.PullDown

    bench.sim_advance(10)
    print('Selecting the client')
    bench.pin_sel.set_external_state('L')
    assert bench.pin_clk.state() == PinState.PullDown
    assert bench.pin_miso.state() == PinState.Low
    assert bench.pin_mosi.state() == PinState.PullDown

    bench.sim_advance(10)
    print('Deselecting the client')
    bench.pin_sel.set_external_state('U')
    assert bench.pin_clk.state() == PinState.PullDown
    assert bench.pin_miso.state() == PinState.PullDown
    assert bench.pin_mosi.state() == PinState.PullDown

    print('End of tests')


def test_avr_spi_host_tranfer(bench):
    '''Checks the transfer with the peripheral in host mode and with
    a simple client
    '''

    SPI = bench.dev.SPI
    PORTB = bench.dev.PORTB

    client = SPISimpleClient()
    client.clock_line.attach(bench.pin_clk)
    client.miso_line.attach(bench.pin_miso)
    client.mosi_line.attach(bench.pin_mosi)
    client.select_line.attach(bench.pin_sel)
    client.enabled = True

    #Set PB5 (SCK), PB3 (MOSI), PB2 (CS) as outputs, and set CS high
    PORTB.PORT = 0x04
    PORTB.DDR = 0x2C
    #Enable the peripheral in host mode
    with SPI.SPCR:
        SPI.SPCR.MSTR = True
        SPI.SPCR.SPE = True

    for i in range(0, 256, 7):
        for b in corelib.SPI.BitOrder:
            for m in corelib.SPI.SerialMode:
                #Set the host and the client in the same config
                client.set_serial_mode(m)
                client.set_bit_order(b)
                with SPI.SPCR:
                    SPI.SPCR.CPOL = bool(m.value & 0x02)
                    SPI.SPCR.CPHA = bool(m.value & 0x01)
                    SPI.SPCR.DORD = (b == corelib.SPI.BitOrder.LSBFirst)

                #transfer a byte in both direction and check the received value at both ends
                client.set_miso_frame(255 - i)
                PORTB.PORT = 0x00
                SPI.SPDR = i
                bench.sim_advance(90)
                PORTB.PORT = 0x04
                bench.sim_advance(10)
                assert client.mosi_frame == i
                assert SPI.SPDR == 255 - i


def test_avr_spi_host_select(bench):
    '''Check the Select line behaviour in host mode
    '''

    SPI = bench.dev.SPI
    PORTB = bench.dev.PORTB
    #Set PB5 (SCK), PB3 (MOSI), PB2 (CS) as outputs, and set CS high
    #Set CS as input with pullup
    PORTB.PORT = 0x04
    PORTB.DDR = 0x28
    #Enable the peripheral in host mode
    with SPI.SPCR:
        SPI.SPCR.MSTR = True
        SPI.SPCR.SPE = True

    assert SPI.SPCR.MSTR
    assert not SPI.SPSR.SPIF

    bench.pin_sel.set_external_state('L')

    assert not SPI.SPCR.MSTR
    assert SPI.SPSR.SPIF


def test_avr_spi_intflag(bench):
    '''Check the interrupt flag logic
    '''

    SPI = bench.dev.SPI
    with SPI.SPCR:
        SPI.SPCR.MSTR = True
        SPI.SPCR.SPE = True

    #Check that SPIF is set after a transfer and is clearedby reading first SPSR then SPDR
    assert not SPI.SPSR.SPIF
    SPI.SPDR = 0x00
    bench.sim_advance(50)
    assert SPI.SPSR.SPIF
    SPI.SPDR.read()
    assert not SPI.SPSR.SPIF

    #Check that when SPIE is set, the ISR is executed
    SPI.SPCR.SPIE = True
    SPI.SPDR = 0x00
    bench.sim_advance(50)
    assert not SPI.SPSR.SPIF
    assert bench.dev.CPU.GPIOR0 == 17 #index of the SPI interrupt vector


def test_avr_spi_wcol(bench):
    '''Check the Write Collision protection
    '''

    SPI = bench.dev.SPI
    with SPI.SPCR:
        SPI.SPCR.MSTR = True
        SPI.SPCR.SPE = True

    #Check that SPIF is set after a transfer and is clearedby reading first SPSR then SPDR
    SPI.SPDR = 0xAA
    bench.sim_advance(10)
    SPI.SPDR = 0x55
    assert SPI.SPSR.WCOL
