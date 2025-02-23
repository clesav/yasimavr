# test_core_usart.py
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
Tests for the USART generic model
'''

import pytest
import yasimavr.lib.core as corelib

UART = corelib.UART



class _TestUSART(UART.USART):

    def __init__(self, cycler, logger):
        super().__init__()
        self._lines = [True, True, False, False]
        self.init(cycler, logger)
        self._cycler = cycler

    def reset(self):
        super().reset()
        self._lines = [True, True, False, False]
        self.signal().raise_(10, 1, 0)
        self.signal().raise_(10, 1, 1)
        self.signal().raise_(10, 0, 2)
        self.signal().raise_(10, 0, 3)

    @property
    def lineTXD(self): return self._lines[UART.Line.TXD]
    @property
    def lineRXD(self): return self._lines[UART.Line.RXD]
    @property
    def lineXCK(self): return self._lines[UART.Line.XCK]
    @property
    def lineDIR(self): return self._lines[UART.Line.DIR]

    def setLineRXD(self, v):
        self.set_line_state(UART.Line.RXD, bool(v))

    def setLineXCK(self, v):
        self.set_line_state(UART.Line.XCK, bool(v))

    def set_line_state(self, line, state):
        old_state = self._lines[line]
        self._lines[line] = state
        if state != old_state:
            self.line_state_changed(line, state)
            self.signal().raise_(10, 1 if state else 0, int(line))

    def get_line_state(self, line):
        return self._lines[line]


class UsartSignalHook(corelib.SignalHook):

    def __init__(self, cycler, signal=None, tag=0):
        super().__init__()
        self._cycler = cycler
        if signal is not None:
            signal.connect(self, tag)
        self.data = None
        self.frame = None

    def raised(self, sigdata, tag):
        if sigdata.sigid == 10:
            msg = "LineChange %s = %d" % (UART.Line(sigdata.index), sigdata.data.value())
        else:
            msg = UART.SignalId(sigdata.sigid)
            if sigdata.sigid == UART.SignalId.TX_Data:
                self.data = sigdata.data.value()
            elif sigdata.sigid == UART.SignalId.TX_Frame:
                self.frame = sigdata.data.value()

        print('[%08d] Signal %s' % (self._cycler.cycle(), msg))


class _Bench():

    def __init__(self):
        self.cycler = corelib.CycleManager()
        self.log_handler = corelib.LogHandler()
        self.log_handler.init(self.cycler)
        self.logger = corelib.Logger(0, self.log_handler)
        #self.logger.set_level(corelib.Logger.Level.Debug)

    def sim_advance(self, nbcycles, expect_end=False):
        final_cycle = self.cycler.cycle() + nbcycles
        while self.cycler.cycle() < final_cycle:
            next_cycle = min(self.cycler.next_when(), final_cycle)
            if next_cycle < 0: break
            self.cycler.increment_cycle(next_cycle - self.cycler.cycle())
            self.cycler.process_timers()


@pytest.fixture
def bench_usart():
    bench = _Bench();

    usart1 = _TestUSART(bench.cycler, bench.logger)
    usart1.reset()

    usart2 = _TestUSART(bench.cycler, bench.logger)
    usart2.reset()

    bench.hook = UsartSignalHook(bench.cycler, usart1.signal())

    return bench, usart1, usart2


def test_core_usart_tx_sync(bench_usart):
    '''Check a typical transmission in synchronous mode
    '''

    bench, usart1, usart2 = bench_usart

    usart1.set_clock_mode(UART.ClockMode.Emitter)
    usart1.set_bit_delay(10)
    usart2.set_clock_mode(UART.ClockMode.Receiver)
    usart2.set_rx_enabled(True)
    usart1.push_tx(0x11)

    assert usart1.tx_in_progress()
    assert usart1.tx_pending() == 0

    for i in range(60):
        usart2.setLineRXD(usart1.lineTXD)
        usart2.setLineXCK(usart1.lineXCK)
        bench.sim_advance(1)

    assert usart2.rx_in_progress()

    for i in range(60):
        usart2.setLineRXD(usart1.lineTXD)
        usart2.setLineXCK(usart1.lineXCK)
        bench.sim_advance(1)

    assert not usart1.tx_in_progress()
    assert usart2.rx_available()
    assert usart2.read_rx() == 0x11
    usart2.pop_rx()
    assert not usart2.rx_available()


def test_core_usart_tx_async(bench_usart):
    '''Check a typical transmission in asynchronous mode
    '''

    bench, usart1, usart2 = bench_usart

    usart1.set_clock_mode(UART.ClockMode.Async)
    usart1.set_bit_delay(10)
    usart2.set_clock_mode(UART.ClockMode.Async)
    usart2.set_bit_delay(10)
    usart2.set_rx_enabled(True)
    usart1.push_tx(0x22)

    assert usart1.tx_in_progress()
    assert usart1.tx_pending() == 0

    for i in range(60):
        usart2.setLineRXD(usart1.lineTXD)
        bench.sim_advance(1)

    assert usart2.rx_in_progress()

    for i in range(60):
        usart2.setLineRXD(usart1.lineTXD)
        bench.sim_advance(1)

    assert not usart1.tx_in_progress()
    assert usart2.rx_available()
    assert usart2.read_rx() == 0x22
    usart2.pop_rx()
    assert not usart2.rx_available()


def test_core_usart_frame_format(bench_usart):
    '''Check various frame formatbased on databits/parity/stopbits settings
    '''

    bench, usart1, usart2 = bench_usart

    usart1.set_clock_mode(UART.ClockMode.Async)
    usart1.set_bit_delay(10)
    usart1.push_tx(0x22)

    bench.sim_advance(100)
    assert bench.hook.frame == 0x244

    usart1.set_parity(UART.Parity.Odd)
    usart1.push_tx(0x22)
    bench.sim_advance(100)
    assert bench.hook.frame == 0x444

    usart1.set_parity(UART.Parity.Even)
    usart1.push_tx(0x22)
    bench.sim_advance(100)
    assert bench.hook.frame == 0x644

    usart1.set_parity(UART.Parity.No)
    usart1.set_databits(5)
    usart1.push_tx(0x66)
    bench.sim_advance(100)
    assert bench.hook.frame == 0x04C

    usart1.set_databits(9)
    usart1.push_tx(0x66)
    bench.sim_advance(100)
    assert bench.hook.frame == 0x4CC

    usart1.set_stopbits(2)
    usart1.push_tx(0x66)
    bench.sim_advance(100)
    assert bench.hook.frame == 0xCCC


def test_core_usart_parity_flag(bench_usart):
    '''Check the parity flag logic
    '''

    bench, usart1, usart2 = bench_usart

    usart1.set_bit_delay(10)
    usart1.set_parity(UART.Parity.Odd)
    usart2.set_bit_delay(10)
    usart2.set_parity(UART.Parity.Even)
    usart2.set_rx_enabled(True)
    usart1.push_tx(0x11)
    for i in range(200):
        usart2.setLineRXD(usart1.lineTXD)
        bench.sim_advance(1)

    assert bench.hook.frame == 0x422
    assert usart2.read_rx() == 0x11
    assert usart2.has_parity_error()
    usart2.pop_rx()
    assert not usart2.has_parity_error()

    usart1.set_parity(UART.Parity.Even)
    usart1.push_tx(0x11)
    for i in range(200):
        usart2.setLineRXD(usart1.lineTXD)
        bench.sim_advance(1)

    assert usart2.read_rx() == 0x11
    assert not usart2.has_parity_error()


def test_core_usart_bytewise(bench_usart):
    '''Check the byte push feature
    '''

    bench, usart1, usart2 = bench_usart

    usart1.set_rx_enabled(True)
    usart1.push_rx_frame(0x55)

    assert not usart1.rx_available()
    bench.sim_advance(100)
    assert usart1.rx_available()
    assert usart1.read_rx() == 0x55
