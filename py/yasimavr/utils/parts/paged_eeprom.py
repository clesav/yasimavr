# paged_eeprom.py
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
This module defines a simple paged eeprom using a TWI/I2C interface.
It is originally based on devices AT24Cxx from Microchip.
'''


from yasimavr.lib import core as _corelib
from yasimavr.utils.twi import TWISimpleClient


class PagedEEPROM(TWISimpleClient):

    def __init__(self, address, page_size, page_count, write_delay, initial_data=None):
        '''Initialise a paged EEPROM
        \param address I2C address
        \param page_size storage page size in bytes
        \param page_count storage page count
        \param write_delay duration of a write operation in simulation cycles
        \param initial_data optional data to initialise the storage.

        The initial data can be an int (which the storage is filled with) or an iterable of bytes.
        If not specified, the storage is initially filled with zeros.
        '''

        super().__init__(address)

        self._page_size = page_size
        self._page_count = page_count
        self._write_delay = write_delay

        storage_size = page_size * page_count
        self._storage = bytearray(storage_size)
        if isinstance(initial_data, int):
            for i in range(storage_size):
                self._storage[i] = initial_data
        elif initial_data is not None:
            n = min(storage_size, len(initial_data))
            self._storage[:n] = initial_data[:n]

        self._buffer = bytearray(page_size)
        self._bufset = bytearray(page_size)
        for i in range(page_size):
            self._buffer[i] = self._bufset[i] = 0

        self._page_num = 0
        self._page_offset = 0
        self._byte_counter = 0
        self._cycle_manager = None
        self._write_timer = _corelib.CallableCycleTimer(self._write_timer_next)


    def init(self, cycle_manager):
        '''Initialise with the simulation loop.
        Must be called before any simulation is run.
        '''

        super().init(cycle_manager)
        self._cycle_manager = cycle_manager
        self.set_enabled(True)


    #override of TWISimpleClient
    def address_match(self, addr_rw):
        #Simple address match but don't acknowledge if a writing cycle is in progress
        return super().address_match(addr_rw) and not self._write_timer.scheduled()


    #override of TWISimpleClient
    def transfer_start(self, rw):
        self._byte_counter = 0
        for i in range(self._page_size):
            self._bufset[i] = 0


    #override of TWISimpleClient
    def read_handler(self):
        data = self._storage[self._page_num * self._page_size + self._page_offset]

        if self._page_offset < self._page_size - 1:
            self._page_offset += 1
        else:
            self._page_offset = 0
            self._page_num = (self._page_num + 1) % self._page_count

        return data


    #override of TWISimpleClient
    def write_handler(self, data):
        #the first two bytes are the storage address, MSB first
        if self._byte_counter < 2:
            addr = self._page_num * self._page_size + self._page_offset

            if self._byte_counter:
                addr = (addr & 0xFF00) | data
            else:
                addr = (addr & 0x00FF) | (data << 8)

            self._page_offset = addr % self._page_size
            self._page_num = (addr // self._page_size) % self._page_count

        #the rest is data bytes
        else:
            #write the data byte into the page buffer
            self._buffer[self._page_offset] = data
            self._bufset[self._page_offset] = 1
            #increment the data address, with roll over at the start the current page
            self._page_offset = (self._page_offset + 1) % self._page_size

        self._byte_counter += 1

        #Always respond with ACK
        return True


    #override of TWISimpleClient
    def transfer_stop(self, ok):
        #if any byte has been written
        if ok and any(self._bufset):
            #Copy the written bytes into the storage
            page_start = self._page_num * self._page_size
            for i in range(self._page_size):
                if self._bufset[i]:
                    self._storage[page_start + i] = self._buffer[i]

            #Simulate the delay to write the EEPROM
            self._cycle_manager.delay(self._write_timer, self._write_delay)


    def _write_timer_next(self, _):
        #Write cycle timer callback, nothing to do
        return 0
