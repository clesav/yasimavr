# test_avr_twi.py
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
Automatic tests for the TWI simulation model (AVR architecture)
'''


import pytest
from _test_bench_avr import BenchAVR, PATH_TESTFW_M328
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
        print('Set line state', line, state)
        if line == TWILine.Clock:
            self.scl_line.set_state('U' if state else 'L')
        else:
            self.sda_line.set_state('U' if state else 'L')

    def _signal_raised(self, sigdata, _):
        sid = SIGID(sigdata.sigid)
        if sid == SIGID.AddressReceived:
            self.addr_rw = sigdata.data.value()

        elif sid == SIGID.DataStandby:
            if not self.rw():
                self.start_data_rx()

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

        elif sid == SIGID.DataStandby:
            if self.rw() and (self.ack() or sigdata.data.value()):
                self.start_data_rx()

        elif sid == SIGID.DataReceived:
            self.rx_data = sigdata.data.value()

        elif sid == SIGID.DataAckReceived:
            self.client_ack = bool(sigdata.data.value())


@pytest.fixture
def bench():
    dev_model = load_device('atmega328')
    b = BenchAVR(dev_model, PATH_TESTFW_M328)
    b.pin_scl = b.dev.pins['PC5']
    b.pin_sda = b.dev.pins['PC4']

    b.pin_scl.set_external_state('U')
    b.pin_sda.set_external_state('U')

    #b.dev_model.find_peripheral('TWI0').logger().set_level(corelib.Logger.Level.Trace)
    return b


def _read_status(twi):
    return int(twi.TWSR.TWS) << 3


def _send_command(twi, intr, ack, sta, sto):
    with twi.TWCR:
        twi.TWCR.TWINT = intr
        twi.TWCR.TWEA = ack
        twi.TWCR.TWSTA = sta
        twi.TWCR.TWSTO = sto
        twi.TWCR.TWEN = True


def test_avr_twi_host_write_address_ack(bench):
    '''This test checks the behaviour and flags when a client reports ACK
    to the address
    '''

    TWI = bench.dev.TWI

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.TWBR = 72
    _send_command(TWI, 1, 0, 1, 0)
    assert _read_status(TWI) == 0xF8 #no status

    bench.sim_advance(200)
    assert _read_status(TWI) == 0x08 #START status
    assert TWI.TWCR.TWINT

    TWI.TWDR = 0x42
    _send_command(TWI, 1, 0, 0, 0)
    bench.sim_advance(2000)
    assert client.addr_rw == 0x42
    assert client.clock_hold()
    assert not client.active()
    assert _read_status(TWI) == 0xF8
    assert not TWI.TWCR.TWINT

    client.set_ack(True)
    bench.sim_advance(200)
    assert client.active()
    assert _read_status(TWI) == 0x18 #TX ACK status
    assert TWI.TWCR.TWINT


def test_avr_twi_host_write_address_nack(bench):
    '''This test checks the behaviour and flags when a client reports NACK
    to the address
    '''

    TWI = bench.dev.TWI

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.TWBR = 72
    _send_command(TWI, 1, 0, 1, 0)
    assert _read_status(TWI) == 0xF8

    bench.sim_advance(200)
    assert _read_status(TWI) == 0x08 #START status
    assert TWI.TWCR.TWINT

    TWI.TWDR = 0x42
    _send_command(TWI, 1, 0, 0, 0)
    bench.sim_advance(2000)
    assert client.addr_rw == 0x42
    assert client.clock_hold()
    assert not client.active()
    assert _read_status(TWI) == 0xF8 #no status
    assert not TWI.TWCR.TWINT

    client.set_ack(False)
    bench.sim_advance(200)
    assert not client.active()
    assert _read_status(TWI) == 0x20 #TX NACK status
    assert TWI.TWCR.TWINT


def test_avr_twi_host_write_data(bench):
    '''This test checks the behaviour and flags for a write request with the MCU as host
    '''

    TWI = bench.dev.TWI

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.TWBR = 48
    _send_command(TWI, 1, 0, 1, 0)
    bench.sim_advance(500)
    TWI.TWDR = 0x42
    _send_command(TWI, 1, 0, 0, 0)
    bench.sim_advance(2000)
    client.set_ack(True)
    bench.sim_advance(500)

    TWI.TWDR = 0x61
    _send_command(TWI, 1, 0, 0, 0)
    bench.sim_advance(2000)
    assert client.rx_data == 0x61
    assert client.clock_hold()
    assert client.active()
    assert client.rx_data == 0x61
    assert _read_status(TWI) == 0xF8 #no status
    assert not TWI.TWCR.TWINT

    client.set_ack(True)
    bench.sim_advance(500)
    assert _read_status(TWI) == 0x28 #Data TX ACK status
    assert TWI.TWCR.TWINT

    TWI.TWDR = 0x29
    _send_command(TWI, 1, 0, 0, 0)
    bench.sim_advance(2000)
    assert client.rx_data == 0x29

    client.set_ack(False)
    bench.sim_advance(500)
    assert _read_status(TWI) == 0x30 #Data TX NACK status
    assert TWI.TWCR.TWINT

    _send_command(TWI, 1, 0, 0, 1)
    bench.sim_advance(500)


def test_avr_twi_host_read_data(bench):
    '''This test checks the behaviour and flags for a read request with the MCU as host
    '''

    TWI = bench.dev.TWI

    client = _TestClient(bench.pin_scl, bench.pin_sda)
    client.init(bench.loop.cycle_manager())
    client.set_enabled(True)

    TWI.TWBR = 48
    _send_command(TWI, 1, 0, 1, 0)
    bench.sim_advance(500)
    TWI.TWDR = 0x43
    _send_command(TWI, 1, 1, 0, 0)
    bench.sim_advance(2000)
    client.set_ack(True)
    bench.sim_advance(500)

    assert client.addr_rw == 0x43
    assert client.clock_hold()
    assert client.active()
    assert _read_status(TWI) == 0x40 #ADR RX ACK status
    assert TWI.TWCR.TWINT

    _send_command(TWI, 1, 1, 0, 0)
    client.start_data_tx(0x38)
    bench.sim_advance(2000)
    assert client.clock_hold()
    assert client.active()
    assert client.host_ack is True
    assert _read_status(TWI) == 0x50 #Data RX ACK status
    assert TWI.TWCR.TWINT
    assert TWI.TWDR == 0x38

    _send_command(TWI, 1, 0, 0, 0)
    client.start_data_tx(0x59)
    bench.sim_advance(2000)
    assert client.host_ack is False
    assert _read_status(TWI) == 0x58 #Data RX NACK status
    assert TWI.TWCR.TWINT
    assert TWI.TWDR == 0x59


def test_avr_twi_client_write_data(bench):
    '''This test checks the behaviour and flags for a write request with the MCU as client
    '''

    TWI = bench.dev.TWI

    host = _TestHost(bench.pin_scl, bench.pin_sda)
    host.init(bench.loop.cycle_manager())
    host.set_enabled(True)
    host.set_bit_delay(40)

    TWI.TWBR = 48
    TWI.TWAR.TWA = 0x42
    TWI.TWAMR.TWAM = 0x00
    _send_command(TWI, 1, 1, 0, 0)

    host.start_transfer()
    bench.sim_advance(500)
    host.set_address(0x84)
    bench.sim_advance(2000)
    assert _read_status(TWI) == 0x60 #ADR RX ACK status
    assert TWI.TWCR.TWINT

    _send_command(TWI, 1, 1, 0, 0)
    host.start_data_tx(0x5C)
    bench.sim_advance(2000)
    assert _read_status(TWI) == 0x80 #Data RX ACK status
    assert TWI.TWCR.TWINT
    assert host.client_ack

    _send_command(TWI, 1, 0, 0, 0)
    host.start_data_tx(0x1A)
    bench.sim_advance(2000)
    assert _read_status(TWI) == 0x88 #Data RX NACK status
    assert TWI.TWCR.TWINT
    assert not host.client_ack

    host.stop_transfer()
    bench.sim_advance(100)


def test_avr_twi_client_read_data(bench):
    '''This test checks the behaviour and flags for a read request with the MCU as client
    '''

    TWI = bench.dev.TWI

    host = _TestHost(bench.pin_scl, bench.pin_sda)
    host.init(bench.loop.cycle_manager())
    host.set_enabled(True)
    host.set_bit_delay(40)

    TWI.TWBR = 48
    TWI.TWAR.TWA = 0x42
    TWI.TWAMR.TWAM = 0x00
    _send_command(TWI, 1, 1, 0, 0)

    host.start_transfer()
    bench.sim_advance(500)
    host.set_address(0x85)
    bench.sim_advance(1000)
    assert _read_status(TWI) == 0xA8 #ADR TX ACK status
    assert TWI.TWCR.TWINT

    TWI.TWDR = 0x35
    _send_command(TWI, 1, 1, 0, 0)
    host.start_data_rx()
    bench.sim_advance(1000)
    host.set_ack(True)
    bench.sim_advance(500)
    assert _read_status(TWI) == 0xB8 #Data RX ACK status
    assert TWI.TWCR.TWINT
    assert host.client_ack
    assert host.rx_data == 0x35

    TWI.TWDR = 0x6C
    _send_command(TWI, 1, 1, 0, 0)
    host.start_data_rx()
    bench.sim_advance(1000)
    host.set_ack(False)
    bench.sim_advance(500)
    assert _read_status(TWI) == 0xC0 #Data RX NACK status
    assert TWI.TWCR.TWINT
    assert host.client_ack
    assert host.rx_data == 0x6C

    _send_command(TWI, 1, 0, 0, 0)
    host.stop_transfer()
    bench.sim_advance(500)
