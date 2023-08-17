# uart.py
#
# Copyright 2022 Clement Savergne <csavergne@yahoo.com>
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
See the serial_echo.py example for how to use it.
'''

import collections
import io

import yasimavr.lib.core as _corelib

_UART_SignalId = _corelib.UART.SignalId


class UartIO(io.RawIOBase):

    class _TxHook(_corelib.SignalHook):

        def __init__(self):
            super(UartIO._TxHook, self).__init__()
            self.queue = collections.deque()

        def raised(self, sigdata, _):
            if sigdata.sigid == _UART_SignalId.DataFrame:
                self.queue.append(sigdata.data.value())

    def __init__(self, device, portnum, mode='rw'):
        super(UartIO, self).__init__()

        if mode not in ('rw', 'r', 'w'):
            raise ValueError('Invalid mode')

        portnum = str(portnum)
        ok, reqdata = device.ctlreq(_corelib.IOCTL_UART(portnum), _corelib.CTLREQ_UART_ENDPOINT)

        if not ok:
            raise ValueError('Endpoint of UART port ' + portnum + ' not found')

        self._endpoint = reqdata.data.as_ptr(_corelib.UARTEndPoint)

        #Create the signal hook and connect to the endpoint
        if 'r' in mode:
            self._rx_hook = self._TxHook()
            self._endpoint.tx_signal.connect(self._rx_hook)

        if 'w' in mode:
            self._tx_signal = _corelib.Signal()
            self._tx_signal.connect(self._endpoint.rx_hook)

        self._mode = mode


    def write(self, data):
        if 'w' not in self._mode:
            raise IOError('Invalid mode')
        elif isinstance(data, int) and (0 <= data <= 255):
            self._tx_signal.raise_(_UART_SignalId.DataFrame, data)
        elif isinstance(data, (bytes, bytearray)):
            self._tx_signal.raise_(_UART_SignalId.DataBytes, bytes(data))
        else:
            raise TypeError('Invalid data type')


    def readinto(self, b):
        if 'r' not in self._mode:
            raise IOError('Invalid mode')

        n = min(len(self._rx_hook.queue), len(b))
        for i in range(n):
            b[i] = self._rx_hook.queue.popleft()
        return n


    def readable(self):
        return 'r' in self._mode

    def writable(self):
        return 'w' in self._mode

    def available(self):
        return len(self._rx_hook.queue)


    def close(self):
        if not self.closed:
            self._endpoint.tx_signal.disconnect(self)
            self._tx_signal.disconnect(self._endpoint.rx_hook)
            self._rx_hook.queue.clear()

        super(UartIO, self).close()
