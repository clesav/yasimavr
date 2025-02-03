# test_XT_TWI.py
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
Automatic tests for the TWI simulation model (XT architecture)
'''


import pytest
from _test_bench_xt import BenchXT, TESTFW_M4809
import yasimavr.lib.core as corelib
from yasimavr.device_library import load_device


TWILine = corelib.TWI.Line
SIGID = corelib.TWI.SignalId


class _TestClient(corelib.TWI.Client):

    def __init__(self, scl_line, sda_line):
        super().__init__()

        self.addr_rw = -1
        self.rx_data = -1
        self.host_ack = None

        self.scl_line = scl_line
        self.sda_line = sda_line

        self._line_hook = corelib.CallableSignalHook(self._line_signal_raised)
        scl_line.signal().connect(self._line_hook, TWILine.Clock)
        sda_line.signal().connect(self._line_hook, TWILine.Data)

        self._state_hook = corelib.CallableSignalHook(self._signal_raised)
        self.signal().connect(self._state_hook)

    def _line_signal_raised(self, sigdata, hooktag):
        if sigdata.sigid == corelib.Wire.SignalId.DigitalChange:
            value = sigdata.data.value()
            line = TWILine(hooktag)
            self.line_state_changed(line, value)

    #TWI2Client override
    def set_line_state(self, line, state):
        if line == TWILine.Clock:
            self.scl_line.set_state('U' if state else 'L')
        else:
            self.sda_line.set_state('U' if state else 'L')

    def _signal_raised(self, sigdata, _):
        sid = SIGID(sigdata.sigid)
        if sid == SIGID.DataStandby:
            if (not self.rw()) and (sigdata.data.as_uint() or self.ack()):
                self.start_data_rx();

        elif sid == SIGID.AddressReceived:
            self.addr_rw = sigdata.data.value()

        elif sid == SIGID.DataReceived:
            self.rx_data = sigdata.data.value()

        elif sid == SIGID.DataAckReceived:
            self.host_ack = bool(sigdata.data.value())


class _TestHost(corelib.TWI.Host):

    def __init__(self, scl_line, sda_line):
        super().__init__()

        self.addr_rw = -1
        self.rx_data = -1
        self.client_ack = None

        self.scl_line = scl_line
        self.sda_line = sda_line

        self._line_hook = corelib.CallableSignalHook(self._line_signal_raised)
        scl_line.signal().connect(self._line_hook, TWILine.Clock)
        sda_line.signal().connect(self._line_hook, TWILine.Data)

        self._state_hook = corelib.CallableSignalHook(self._signal_raised)
        self.signal().connect(self._state_hook)

    def _line_signal_raised(self, sigdata, hooktag):
        if sigdata.sigid == corelib.Wire.SignalId.DigitalChange:
            value = sigdata.data.value()
            line = TWILine(hooktag)
            self.line_state_changed(line, value)

    #TWI2Host override
    def set_line_state(self, line, state):
        if line == TWILine.Clock:
            self.scl_line.set_state('U' if state else 'L')
        else:
            self.sda_line.set_state('U' if state else 'L')

    def _signal_raised(self, sigdata, _):
        sid = SIGID(sigdata.sigid)
        if sid == SIGID.AddressSent:
            self.client_ack = bool(sigdata.data.value())

        elif sid == SIGID.DataReceived:
            self.rx_data = sigdata.data.value()


@pytest.fixture
def bench():
    dev_model = load_device('atmega4809')
    b = BenchXT(dev_model, TESTFW_M4809)
    b.pin_scl = b.dev.pins['PA3']
    b.pin_sda = b.dev.pins['PA2']

    b.pin_scl.set_external_state('U')
    b.pin_sda.set_external_state('U')

    #b.dev_model.find_peripheral('TWI0').logger().set_level(corelib.Logger.Level.Trace)
    return b


def test_xt_twi_host_write_address_ack(bench):
    '''This test checks the behaviour and flags when a client reports ACK
    to the address
    '''

    TWI = bench.dev.TWI0

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.MBAUD = 0x55
    TWI.MCTRLA.ENABLE = True
    TWI.MSTATUS.BUSSTATE = 'IDLE'
    TWI.MADDR = 0x42
    bench.sim_advance(2000)

    assert client.addr_rw == 0x42
    assert client.clock_hold()
    assert not client.active()
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x02

    client.set_ack(True)
    bench.sim_advance(200)
    assert client.active()
    #RIF=0, WIF=1, CLKHOLD=1, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x62

    TWI.MSTATUS = 0xFF
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=1, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x02


def test_xt_twi_host_write_address_nack(bench):
    '''This test checks the behaviour and flags when a client reports NACK
    to the address
    '''

    TWI = bench.dev.TWI0

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.MBAUD = 0x55
    TWI.MCTRLA.ENABLE = True
    TWI.MSTATUS.BUSSTATE = 'IDLE'
    TWI.MADDR = 0x42
    bench.sim_advance(2000)

    assert client.addr_rw == 0x42
    client.set_ack(False)
    bench.sim_advance(200)
    assert not client.clock_hold()
    assert not client.active()
    #RIF=0, WIF=1, CLKHOLD=1, RXACK=1, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x72

    TWI.MSTATUS = 0xFF
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=1, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x12


def test_xt_twi_host_write_data(bench):
    '''This test checks the behaviour and flags for a write request with the MCU as host
    '''

    TWI = bench.dev.TWI0

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.MBAUD = 0x55
    TWI.MCTRLA.ENABLE = True
    TWI.MSTATUS.BUSSTATE = 'IDLE'
    TWI.MADDR = 0x42
    bench.sim_advance(2000)

    assert client.addr_rw == 0x42
    client.set_ack(True)
    bench.sim_advance(500)
    assert client.active()
    #RIF=0, WIF=1, CLKHOLD=1, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x62

    TWI.MSTATUS = 0xFF
    TWI.MDATA = 0x61
    bench.sim_advance(2000)
    assert client.clock_hold()
    assert client.active()
    assert client.rx_data == 0x61
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x02

    client.set_ack(True)
    bench.sim_advance(500)
    #RIF=0, WIF=1, CLKHOLD=1, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x62

    TWI.MSTATUS = 0xFF
    TWI.MDATA = 0x29
    bench.sim_advance(2000)
    assert client.rx_data == 0x29

    client.set_ack(False)
    bench.sim_advance(500)
    #RIF=0, WIF=1, CLKHOLD=1, RXACK=1, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x72

    TWI.MCTRLB.MCMD = 'STOP'
    bench.sim_advance(2000)
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=1, ARBLOST=0, BUSERR=0, BUSSTATE=idle
    assert TWI.MSTATUS == 0x11


def test_xt_twi_host_read_data(bench):
    '''This test checks the behaviour and flags for a read request with the MCU as host
    '''

    TWI = bench.dev.TWI0

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.MBAUD = 0x55
    TWI.MCTRLA.ENABLE = True
    TWI.MSTATUS.BUSSTATE = 'IDLE'
    TWI.MADDR = 0x43
    bench.sim_advance(2000)

    assert client.addr_rw == 0x43
    client.set_ack(True)
    bench.sim_advance(500)
    assert client.clock_hold()
    assert client.active()
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x02

    client.start_data_tx(0x38)
    assert not client.clock_hold()
    bench.sim_advance(2000)
    assert not client.clock_hold()
    assert client.active()
    #RIF=1, WIF=0, CLKHOLD=1, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0xA2
    assert TWI.MDATA == 0x38

    TWI.MCTRLB.ACKACT = 0
    TWI.MCTRLB.MCMD = 'RECVTRANS'
    bench.sim_advance(500)
    #RIF=0, WIF=0, CLKHOLD=0, RXACK=0, ARBLOST=0, BUSERR=0, BUSSTATE=owned
    assert TWI.MSTATUS == 0x02
    assert client.host_ack is True


def test_xt_twi_client_write_data(bench):
    '''This test checks the behaviour and flags for a write request with the MCU as client
    '''

    TWI = bench.dev.TWI0

    host = _TestHost(bench.pin_scl, bench.pin_sda)
    host.init(bench.loop.cycle_manager())
    host.set_enabled(True)
    host.set_bit_delay(40)

    TWI.MCTRLA.ENABLE = True
    TWI.SCTRLA.ENABLE = True
    TWI.SCTRLA.PIEN = True
    TWI.MSTATUS.BUSSTATE = 'IDLE'
    TWI.SADDR.ADDR = 0x42
    host.start_transfer()
    bench.sim_advance(100)

    assert TWI.MSTATUS.BUSSTATE == 'BUSY'
    host.set_address(0x42)
    bench.sim_advance(500)
    #DIF=0, APIF=1, CLKHOLD=1, RXACK=0, COLL=0, BUSERR=0, DIR=0, AP=1
    assert TWI.SSTATUS == 0x61
    assert TWI.SDATA == 0x42

    TWI.SCTRLB.ACKACT = 0
    TWI.SCTRLB.SCMD = 'RESPONSE'
    bench.sim_advance(100)
    #DIF=0, APIF=0, CLKHOLD=0, RXACK=0, COLL=0, BUSERR=0, DIR=0, AP=1
    assert TWI.SSTATUS == 0x01
    assert host.client_ack is True

    host.start_data_tx(0x5C)
    bench.sim_advance(500)
    #DIF=0, APIF=0, CLKHOLD=0, RXACK=0, COLL=0, BUSERR=0, DIR=0, AP=1
    assert TWI.SSTATUS == 0xa1

    TWI.SCTRLB.ACKACT = 0
    TWI.SCTRLB.SCMD = 'RESPONSE'
    bench.sim_advance(100)
    #DIF=0, APIF=0, CLKHOLD=0, RXACK=0, COLL=0, BUSERR=0, DIR=0, AP=1
    assert TWI.SSTATUS == 0x01
    assert host.client_ack is True

    TWI.SSTATUS = 0xFF
    host.stop_transfer()
    bench.sim_advance(100)
    #DIF=0, APIF=1, CLKHOLD=0, RXACK=0, COLL=0, BUSERR=0, DIR=0, AP=0
    assert TWI.SSTATUS == 0x40


def test_xt_twi_client_read_data(bench):
    '''This test checks the behaviour and flags for a read request with the MCU as client
    '''

    TWI = bench.dev.TWI0

    host = _TestHost(bench.pin_scl, bench.pin_sda)
    host.init(bench.loop.cycle_manager())
    host.set_enabled(True)
    host.set_bit_delay(40)

    TWI.MCTRLA.ENABLE = True
    TWI.SCTRLA.ENABLE = True
    TWI.SCTRLA.PIEN = True
    TWI.MSTATUS.BUSSTATE = 'IDLE'
    TWI.SADDR.ADDR = 0x43
    host.start_transfer()
    bench.sim_advance(100)

    assert TWI.MSTATUS.BUSSTATE == 'BUSY'
    host.set_address(0x43)
    bench.sim_advance(500)
    #DIF=0, APIF=1, CLKHOLD=1, RXACK=0, COLL=0, BUSERR=0, DIR=1, AP=1
    assert TWI.SSTATUS == 0x63
    assert TWI.SDATA == 0x43

    TWI.SCTRLB.ACKACT = 0
    TWI.SCTRLB.SCMD = 'RESPONSE'
    bench.sim_advance(100)
    #DIF=1, APIF=0, CLKHOLD=1, RXACK=0, COLL=0, BUSERR=0, DIR=1, AP=1
    assert TWI.SSTATUS == 0xA3

    TWI.SDATA = 0x28
    TWI.SCTRLB.SCMD = 'RESPONSE'
    #DIF=0, APIF=0, CLKHOLD=0, RXACK=0, COLL=0, BUSERR=0, DIR=1, AP=1
    assert TWI.SSTATUS == 0x03

    host.start_data_rx()
    bench.sim_advance(500)
    host.set_ack(True)
    bench.sim_advance(100)
    #DIF=1, APIF=0, CLKHOLD=1, RXACK=0, COLL=0, BUSERR=0, DIR=1, AP=1
    assert TWI.SSTATUS == 0xA3
    assert host.rx_data == 0x28

    TWI.SDATA = 0x44
    TWI.SCTRLB.SCMD = 'RESPONSE'
    host.start_data_rx()
    bench.sim_advance(500)
    host.set_ack(False)
    bench.sim_advance(100)
    #DIF=1, APIF=0, CLKHOLD=0, RXACK=1, COLL=0, BUSERR=0, DIR=1, AP=1
    assert TWI.SSTATUS == 0x93
    assert host.rx_data == 0x44

    TWI.SSTATUS = 0xFF
    host.stop_transfer()
    bench.sim_advance(100)
    #DIF=0, APIF=1, CLKHOLD=0, RXACK=1, COLL=0, BUSERR=0, DIR=1, AP=0
    assert TWI.SSTATUS == 0x52
