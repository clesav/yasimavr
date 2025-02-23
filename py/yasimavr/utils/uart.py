# uart.py
#
# Copyright 2022-2025 Clement Savergne <csavergne@yahoo.com>
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
This module defines UartIO which is a reimplementation of io.RawIOBase
that can connect to a AVR device USART peripheral to exchange data with
it as if writing to/reading from a file.
'''

import collections
import io

import yasimavr.lib.core as _corelib

UART = _corelib.UART

__all__ = ['UART', 'UART_IO', 'RawUART']


class UART_IO(io.RawIOBase):
    '''Reimplementation of io.RawIOBase that can connect to a device USART peripheral to exchange data with
    it as if writing to/reading from a file.
    '''

    def __init__(self, device, portnum, mode='rw'):
        super(UART_IO, self).__init__()

        if mode not in ('rw', 'r', 'w'):
            raise ValueError('Invalid mode')

        self._device = device
        self._ctl_id = _corelib.IOCTL_UART(str(portnum))

        #Create the signal hook and connect to the peripheral
        if 'r' in mode:
            self._rx_queue = collections.deque()

            ok, reqdata = device.ctlreq(self._ctl_id, _corelib.CTLREQ_GET_SIGNAL)
            if not ok:
                raise ValueError('UART port ' + str(portnum) + ' not found')

            self._rx_hook = _corelib.CallableSignalHook(self._rx_hook_raised)
            reqdata.data.value(_corelib.Signal).connect(self._rx_hook)

        self._mode = mode


    def _rx_hook_raised(self, sigdata, _):
        if sigdata.sigid == UART.SignalId.TX_Data:
            self._rx_queue.append(sigdata.data.value())


    def write(self, data):
        if 'w' not in self._mode:
            raise IOError('Invalid mode')

        if isinstance(data, int) and (0 <= data <= 255):
            b = bytes((data,))
        elif isinstance(data, (list, bytes, bytearray)):
            b = bytes(data)
        else:
            raise TypeError('Invalid data type')

        reqdata = _corelib.ctlreq_data_t()
        reqdata.data = _corelib.vardata_t(b)
        ok, reqdata = self._device.ctlreq(self._ctl_id, _corelib.CTLREQ_USART_BYTES, reqdata)
        if not ok:
            raise IOError('Error sending data to the peripheral')


    def readinto(self, b):
        if 'r' not in self._mode:
            raise IOError('Invalid mode')

        n = min(len(self._rx_queue), len(b))
        for i in range(n):
            b[i] = self._rx_queue.popleft()
        return n


    def readable(self):
        return 'r' in self._mode


    def writable(self):
        return 'w' in self._mode


    def available(self):
        return len(self._rx_queue) if 'r' in self._mode else 0


    def close(self):
        if not self.closed and 'r' in self._mode:
            self._rx_hook = None
            self._rx_queue.clear()

        super(UART_IO, self).close()


class RawUART(UART.USART):
    '''Simple reimplementation of a USART base class to allow
    the use of UART lines in simulation models.
    #Being bitwise, it is compatible with bit banging interfaces and allow
    to visualise the traffic on the data lines.
    '''

    def __init__(self, ):
        super().__init__()

        self._lineTXD = _corelib.Wire()
        self._lineRXD = _corelib.Wire()
        self._lineXCK = _corelib.Wire()

        self._hook = _corelib.CallableSignalHook(self._wire_signal_raised)
        self._lineTXD.signal().connect(self._hook, UART.Line.TXD)
        self._lineRXD.signal().connect(self._hook, UART.Line.RXD)
        self._lineXCK.signal().connect(self._hook, UART.Line.XCK)


    def get_line_state(self, line):
        if line == UART.Line.TXD:
            return self._lineTXD.digital_state()
        elif line == UART.Line.RXD:
            return self._lineRXD.digital_state()
        elif line == UART.Line.XCK:
            return self._lineXCK.digital_state()
        else:
            return False


    def set_line_state(self, line, state):
        if line == UART.Line.TXD:
            self._lineTXD.set_state('H' if state else 'L')


    def _wire_signal_raised(self, sigdata, hooktag):
        if sigdata.sigid == _corelib.Wire.SignalId.DigitalChange:
            self.line_state_changed(UART.Line(hooktag), sigdata.data.value())


    @property
    def lineTXD(self): return self._lineTXD


    @property
    def lineRXD(self): return self._lineRXD


    @property
    def lineXCK(self): return self._lineXCK
