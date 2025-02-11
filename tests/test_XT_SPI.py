# test_XT_SPI.py
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
Automatic tests for the SPI simulation model (XT architecture)
'''


import pytest
from _test_bench_xt import BenchXT, TESTFW_M4809
from _test_utils import PinState
import yasimavr.lib.core as corelib
from yasimavr.device_library import load_device
from yasimavr.utils.spi import SPISimpleClient


@pytest.fixture
def bench():
    dev_model = load_device('atmega4809')
    b = BenchXT(dev_model, TESTFW_M4809)
    b.pin_clk = b.dev.pins['PA6']
    b.pin_miso = b.dev.pins['PA5']
    b.pin_mosi = b.dev.pins['PA4']
    b.pin_sel = b.dev.pins['PA7']

    b.pin_clk.set_external_state('D')
    b.pin_miso.set_external_state('D')
    b.pin_mosi.set_external_state('D')
    b.pin_sel.set_external_state('U')

    return b


def test_xt_spi_pin_drive(bench):
    SPI = bench.dev.SPI0
    PORTA = bench.dev.PORTA

    PORTA.OUT = 0xFF
    PORTA.DIR = 0xFF

    assert SPI.CTRLA.ENABLE == False
    assert bench.pin_clk.state() == PinState.High
    assert bench.pin_miso.state() == PinState.High
    assert bench.pin_mosi.state() == PinState.High

    bench.sim_advance(10)
    print('Setting host mode')
    SPI.CTRLA.MASTER = True
    SPI.CTRLA.ENABLE = True
    assert bench.pin_clk.state() == PinState.Low
    assert bench.pin_miso.state() == PinState.PullDown
    assert bench.pin_mosi.state() == PinState.Low

    bench.sim_advance(10)
    print('Setting client mode')
    SPI.CTRLA.MASTER = False
    assert bench.pin_clk.state() == PinState.PullDown
    assert bench.pin_miso.state() == PinState.PullDown
    assert bench.pin_mosi.state() == PinState.PullDown

    bench.sim_advance(10)
    print('Selecting the client')
    bench.pin_sel.set_external_state(PinState.Low)
    assert bench.pin_clk.state() == PinState.PullDown
    assert bench.pin_miso.state() == PinState.Low
    assert bench.pin_mosi.state() == PinState.PullDown


def test_xt_spi_host_tranfer(bench):
    SPI = bench.dev.SPI0
    PORTA = bench.dev.PORTA

    client = SPISimpleClient()
    client.clock_line.attach(bench.pin_clk)
    client.miso_line.attach(bench.pin_miso)
    client.mosi_line.attach(bench.pin_mosi)
    client.select_line.attach(bench.pin_sel)
    client.enabled = True

    PORTA.OUT = 0x80
    PORTA.DIR = 0xD0
    SPI.CTRLA.ENABLE = 1
    SPI.CTRLA.MASTER = 1

    for i in range(0, 256, 7):
        for b in corelib.SPI.BitOrder:
            for m in corelib.SPI.SerialMode:
                client.set_serial_mode(m)
                client.set_bit_order(b)
                SPI.CTRLB.MODE = m.value
                SPI.CTRLA.DORD = b.value

                client.set_miso_frame(255 - i)
                PORTA.OUTCLR = 0x80
                SPI.DATA = i
                bench.sim_advance(50)
                PORTA.OUTSET = 0x80
                bench.sim_advance(10)
                assert client.mosi_frame == i
                assert SPI.DATA == 255 - i


def test_xt_spi_host_select(bench):
    '''Check the Client Select Disable feature
    '''
    SPI = bench.dev.SPI0
    PORTA = bench.dev.PORTA
    SPI.CTRLA.ENABLE = 1
    SPI.CTRLA.MASTER = 1
    SPI.CTRLB.SSD = 1

    bench.pin_sel.set_external_state('L')
    assert SPI.CTRLA.MASTER
    assert not SPI.INTFLAGS.IF

    #Check that with SSD=0, the peripheral switches to Client mode on CS negative edge
    bench.pin_sel.set_external_state('U')
    SPI.CTRLB.SSD = 0
    bench.pin_sel.set_external_state('L')
    assert not SPI.CTRLA.MASTER
    assert SPI.INTFLAGS.IF

    #Same check as above but in Buffer mode
    bench.pin_sel.set_external_state('U')
    #Clear INTFLAGS.IF
    SPI.INTFLAGS.IF = 1
    SPI.DATA.read()
    SPI.CTRLA.MASTER = 1
    SPI.CTRLB.BUFEN = 1
    bench.pin_sel.set_external_state('L')
    assert not SPI.CTRLA.MASTER
    assert not SPI.INTFLAGS.IF
    assert SPI.INTFLAGS.SSIF


def test_xt_spi_intflag_IF(bench):
    SPI = bench.dev.SPI0
    SPI.CTRLA.ENABLE = 1
    SPI.CTRLA.MASTER = 1
    SPI.INTCTRL = 0xFF

    assert not SPI.INTFLAGS.IF
    SPI.DATA = 0x00
    bench.sim_advance(100)
    assert SPI.INTFLAGS.IF

    SPI.INTFLAGS.IF = 1
    assert SPI.INTFLAGS.IF
    SPI.DATA.read()
    assert not SPI.INTFLAGS.IF


def test_xt_spi_intflag_WCOL(bench):
    SPI = bench.dev.SPI0
    SPI.CTRLA.ENABLE = 1
    SPI.CTRLA.MASTER = 1
    SPI.INTCTRL = 0xFF
    SPI.DATA = 0xAA
    bench.sim_advance(10)
    SPI.DATA = 0x55
    assert SPI.INTFLAGS.WRCOL


def test_xt_spi_intflag_BUFOVF(bench):
    SPI = bench.dev.SPI0
    SPI.CTRLA.ENABLE = 1
    SPI.CTRLA.MASTER = 1
    SPI.CTRLB.BUFEN = 1

    SPI.DATA = 0x01
    bench.sim_advance(100)
    assert not SPI.INTFLAGS.BUFOVF
    SPI.DATA = 0x02
    bench.sim_advance(100)
    assert not SPI.INTFLAGS.BUFOVF
    SPI.DATA = 0x03
    bench.sim_advance(100)
    assert not SPI.INTFLAGS.BUFOVF
    SPI.DATA = 0x04
    assert SPI.INTFLAGS.BUFOVF

    SPI.INTFLAGS.BUFOVF = 1
    assert not SPI.INTFLAGS.BUFOVF


def test_xt_spi_intflag_RXCIF(bench):
    SPI = bench.dev.SPI0
    SPI.CTRLA.ENABLE = 1
    SPI.CTRLA.MASTER = 1
    SPI.CTRLB.BUFEN = 1

    assert not SPI.INTFLAGS.RXCIF
    SPI.DATA = 0x01
    bench.sim_advance(100)
    assert SPI.INTFLAGS.RXCIF

    SPI.DATA = 0x02
    bench.sim_advance(100)
    assert SPI.INTFLAGS.RXCIF

    SPI.DATA.read()
    assert SPI.INTFLAGS.RXCIF
    SPI.DATA.read()
    assert not SPI.INTFLAGS.RXCIF
