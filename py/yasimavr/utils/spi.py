# spi.py
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
This module defines SPISimpleClient and SPISimpleHost which are simple
instance of SPI devices that can be used for SPI parts simulation.
'''

import collections
import yasimavr.lib.core as _corelib
from yasimavr.lib.core import SPI


__all__ = ['SPI', 'SPISimpleClient', 'SPISimpleHost']


class SPISimpleClient(SPI.EndPoint):
    '''Simple implementation of a SPI client
    '''

    def __init__(self, mode=SPI.SerialMode.Mode0, bitorder=SPI.BitOrder.MSBFirst):
        '''Initialisation of a SimpleClient
        :param mode SPI mode (one of SPI.SerialMode enum values)
        :param bitorder Bit order mode (one of SPI.BitOrder enum values)
        '''

        super().__init__()
        self._mosi_frame = -1
        self._miso_level = False
        self._enabled = False

        self._clock_line = _corelib.Wire()
        self._miso_line = _corelib.Wire()
        self._mosi_line = _corelib.Wire()
        self._select_line = _corelib.Wire()

        self._clock_hook = _corelib.CallableSignalHook(self._clock_hook_raised)
        self._clock_line.signal().connect(self._clock_hook)

        self._select_hook = _corelib.CallableSignalHook(self._select_hook_raised)
        self._select_line.signal().connect(self._select_hook)

        self.set_serial_mode(mode)
        self.set_bit_order(bitorder)


    def _get_enabled(self):
        return self._enabled

    def _set_enabled(self, e):
        self._enabled = e
        if not e:
            self.set_active(False)
            self._update_miso()

    enabled = property(_get_enabled, _set_enabled)


    def _clock_hook_raised(self, sigdata, _):
        if sigdata.sigid == _corelib.Wire.SignalId.DigitalChange:
            state = sigdata.data.as_uint()
            self.set_shift_clock(state)


    def _select_hook_raised(self, sigdata, _):
        if sigdata.sigid == _corelib.Wire.SignalId.DigitalChange:
            state = sigdata.data.as_uint()
            self.set_active(self._enabled and not state)
            self._update_miso()


    #Override of EndPoint
    def write_data_output(self, level):
        self._miso_level = level
        self._update_miso()


    def _update_miso(self):
        if self.active():
            self._miso_line.set_state('H' if self._miso_level else 'L')
        else:
            self._miso_line.set_state('Z')


    #Override of EndPoint
    def read_data_input(self):
        return self._mosi_line.digital_state()


    def frame_completed(self):
        self._mosi_frame = self.shift_data()


    def set_miso_frame(self, frame):
        '''Set the frame to be sent at the next transfer
        '''
        self.set_shift_data(frame)


    @property
    def mosi_frame(self):
        '''Return the last frame received.
        '''
        return self._mosi_frame

    @property
    def clock_line(self):
        return self._clock_line

    @property
    def miso_line(self):
        return self._miso_line

    @property
    def mosi_line(self):
        return self._mosi_line

    @property
    def select_line(self):
        return self._select_line


class SPISimpleHost(SPI.EndPoint):
    '''Simple implementation of a SPI host
    It features a TX FIFO and a RX FIFO to queue frames
    Note that it doesn't manage the chip select line.
    '''

    def __init__(self, cycle_manager, bitdelay, mode=SPI.SerialMode.Mode0, bitorder=SPI.BitOrder.MSBFirst):
        '''Initialisation of a SimpleHost
        :param cycle_manager CycleManager instance associated with the simulation loop
        :param bitdelay duration of a bit in simulation cycles
        :param mode SPI mode (one of SPI.SerialMode enum values)
        :param bitorder Bit order mode (one of SPI.BitOrder enum values)
        '''
        super().__init__()
        self._cycle_manager = cycle_manager
        self.set_serial_mode(mode)
        self.set_bit_order(bitorder)
        self._delay = bitdelay / 2

        self._clock_line = _corelib.Wire()
        self._miso_line = _corelib.Wire()
        self._mosi_line = _corelib.Wire()

        self._miso_fifo = collections.deque()
        self._mosi_fifo = collections.deque()

        self._write_clock_output(mode >= SPI.Mode.Mode2)

        self._mosi_line.set_state('L')

        self._timer = _corelib.CallableCycleTimer(self._next_bit)


    #Override of EndPoint
    def write_data_output(self, state):
        if state == _WireState.Floating:
            state = _WireState.Low
        self._mosi_line.set_state(state)


    #Override of EndPoint
    def read_data_input(self):
        return self._miso_line.digital_state()


    def _write_clock_output(self, level):
        self.set_shift_clock(level)
        self._clock_line.set_state('H' if level else 'L')


    #Override of EndPoint. Push the received frame into the RX buffer
    def frame_completed(self):
        self._miso_fifo.append(self.shift_data())


    def transfer_ended(self):
        '''Callback that can be overriden by sub-classes to be notified when
        the transfer of one frame has completed.
        '''
        pass


    def rx_available(self):
        '''Return the number of frame stored in the RX FIFO buffer.
        '''
        return len(self._miso_fifo)


    def peek_miso_frame(self):
        '''Return the front frame in the RX FIFO buffer without popping it.
        '''
        if not len(self._miso_fifo):
            raise ValueError()
        return self._miso_fifo[0]


    def pop_miso_frame(self):
        '''Pop the front frame in the RX FIFO buffer and return it.
        '''
        if not len(self._miso_fifo):
            raise ValueError()
        return self._miso_fifo.pop_left()


    def start_transfer(self, mosi_frame):
        '''Start a transfer of a SPI frame
        If a transfer is already in progress, the frame is added to the
        TX FIFO buffer and transmitted after the current one.
        '''

        if self.active():
            self._mosi_fifo.append(mosi_frame)
        else:
            self.set_shift_data(mosi_frame)
            self.set_active(True)
            self._cycle_manager.delay(self._timer, self._delay)


    def _next_bit(self, when):
        self._write_clock_output(not self.shift_clock())

        if self.frame_complete():
            self.transfer_ended()
            if len(self._mosi_fifo):
                mosi_frame = self._mosi_fifo.popleft()
                self.set_shift_data(mosi_frame)
                return when + self._delay
            else:
                self.set_active(False)
                return 0
        else:
            return when + self._delay


    def cancel(self):
        '''Cancel the current transfer.
        If a transfer is already in progress, the frame is added to the
        TX FIFO buffer and transmitted after the current one.
        '''

        if self.active():
            self._cycle_manager.cancel(self._timer)
            self.set_active(False)
            self._write_clock_output(self.serial_mode() >= SPI.Mode.Mode2)


    def reset(self):
        '''Cancel the current transfer and clear the FIFO buffers.
        '''
        self.cancel()
        self._miso_fifo.clear()
        self._mosi_fifo.clear()


    def transferring(self):
        '''Returns True if a transfer is in progress.
        '''
        return self.active()

    @property
    def clock_line(self):
        return self._clock_line

    @property
    def miso_line(self):
        return self._miso_line

    @property
    def mosi_line(self):
        return self._mosi_line
