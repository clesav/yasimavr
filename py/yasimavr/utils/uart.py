# uart.py
#
# Copyright 2021 Clement Savergne <csavergne@yahoo.com>
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


class UartIO(io.RawIOBase):
    
    class _TxHook(_corelib.AVR_SignalHook):
        
        def __init__(self):
            super(UartIO._TxHook, self).__init__()
            self.queue = collections.deque()
        
        def raised(self, data, _):
            if data.index == _corelib.UART_SignalIndex.UART_Data_Frame:
                self.queue.append(data.u)
    
    def __init__(self, device, portnum):
        super(UartIO, self).__init__()
        
        portnum = str(portnum)
        ok, reqdata = device.ctlreq(_corelib.IOCTL_UART(portnum), _corelib.CTLREQ_UART_ENDPOINT)
        
        if not ok:
            raise ValueError('Endpoint of UART port ' + portnum + ' not found')
        
        self._endpoint = reqdata.ptr(_corelib.UART_EndPoint)
        
        #Create the signal hook and connect to the endpoint
        self._tx_hook = self._TxHook()
        self._endpoint.tx_signal.connect_hook(self._tx_hook)
        self._tx_signal = _corelib.AVR_Signal()
        self._tx_signal.connect_hook(self._endpoint.rx_hook)
    
    
    def write(self, data):
        if isinstance(data, int):
            self._tx_signal.raise_(0, _corelib.UART_SignalIndex.UART_Data_Frame, data);
        elif isinstance(data, (bytes, bytearray)):
            sigdata = _corelib.signal_data_t(0, _corelib.UART_SignalIndex.UART_Data_String, data)
            self._tx_signal.raise_(sigdata)
        else:
            raise TypeError()
    
    
    def readinto(self, b):
        n = min(len(self._tx_hook.queue), len(b))
        for i in range(n):
            b[i] = self._tx_hook.queue.popleft()
        return n
    
    
    def readable(self):
        return True
    
    def writable(self):
        return True
    
    def available(self):
        return len(self._tx_hook.queue)
    
    
    def close(self):
        if not self.closed:
            self._endpoint.tx_signal.disconnect_hook(self)
            self._tx_signal.disconnect_hook(self._endpoint.rx_hook)
            self._tx_hook.queue.clear()
        
        super(UartIO, self).close()
        
            